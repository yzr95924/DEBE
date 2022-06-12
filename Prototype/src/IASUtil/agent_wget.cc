/*

Copyright 2018 Intel Corporation

This software and the related documents are Intel copyrighted materials,
and your use of them is governed by the express license under which they
were provided to you (License). Unless the License provides otherwise,
you may not use, modify, copy, publish, distribute, disclose or transmit
this software or the related documents without Intel's prior written
permission.

This software and the related documents are provided as is, with no
express or implied warranties, other than those that are expressly stated
in the License.

*/

#include "../../include/IAS/agent_wget.h"
#include "../../include/IAS/common.h"
#include "../../include/IAS/httpparser/httpresponseparser.h"
#include "../../include/IAS/httpparser/response.h"
#include "../../include/IAS/iasrequest.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
using namespace httpparser;

#include <string>
#include <vector>

static vector<string> wget_args;

char verbose_agent = 0;
char debug_agent = 0;

#define CHUNK_SZ 8192
#define WGET_NO_ERROR 0
#define WGET_SERVER_ERROR 8
#define WGET_AUTH_ERROR 6

string AgentWget::name = "wget";

int AgentWget::request(string const& url, string const& post,
    Response& response)
{
    HttpResponseParser parser;
    int pipefd[2];
    pid_t pid;
    string arg;
    string sresponse;
    int status;
    char buffer[CHUNK_SZ];
    size_t bread;
    int repeat;
    int rv = 1;
    char tmpfile[] = "/tmp/wgetpostXXXXXX";
    int postdata = 0;

    if (post.length() > 0) {
        int fd = mkstemp(tmpfile);
        size_t bwritten, rem;
        const char* bp = post.c_str();

        // fprintf(stdout, "+++ POST data written to %s\n", tmpfile);

        postdata = 1;

        if (fd == -1) {
            perror("mkstemp");
            return 0;
        }

        rem = post.length();
        while (rem) {
        retry_write:
            bwritten = write(fd, bp, rem);
            if (bwritten == -1) {
                if (errno == EINTR)
                    goto retry_write;
                else {
                    perror("write");
                    close(fd);
                    unlink(tmpfile);
                    return 0;
                }
            }
            rem -= bwritten;
            bp += bwritten;
        }

        close(fd);
    }

    // Only need to initialize these once
    if (wget_args.size() == 0) {
        wget_args.push_back("wget");

        // Output options

        if (!verbose_agent)
            wget_args.push_back("--quiet");
        wget_args.push_back("--output-document=-");
        wget_args.push_back("--save-headers");
        wget_args.push_back("--content-on-error");
        wget_args.push_back("--no-http-keep-alive");

        arg = conn->proxy_server();
        // Override environment
        if (arg != "") {
            string proxy_url = "http://";
            proxy_url += arg;
            if (conn->proxy_port() != 80) {
                proxy_url += ":";
                proxy_url += to_string(conn->proxy_port());
            }
            proxy_url += "/";

            setenv("https_proxy", proxy_url.c_str(), 1);
        }

        if (conn->proxy_mode() == IAS_PROXY_NONE) {
            wget_args.push_back("--no-proxy");
        } else if (conn->proxy_mode() == IAS_PROXY_FORCE) {
            unsetenv("no_proxy");
        }
    }

    /* Set up two pipes for reading from the child */

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 0;
    }

    /* Spawn the child process */

    pid = fork();
    if (pid == -1) { /* oops */
        perror("fork");
        return 0;
    } else if (pid == 0) { /* child */
        char** argv;
        size_t sz, i;

        // Add instance-specific options

        // construct then add the Ocp-Apim-Subscription-Key subscription key header
        string subscriptionKeyHeader = "--header=Ocp-Apim-Subscription-Key: ";
        subscriptionKeyHeader.append(conn->getSubscriptionKey());
        wget_args.push_back(subscriptionKeyHeader.c_str());

        if (postdata) {

            string contentTypeHeader = "--header=Content-Type: application/json";
            wget_args.push_back(contentTypeHeader.c_str());
            arg = "--post-file=";
            arg += tmpfile;
            wget_args.push_back(arg);
        }

        // Add the url

        wget_args.push_back(url.c_str());

        sz = wget_args.size();

        // Create the argument list

        if (debug_agent)
            eprintf("+++ Exec:");
        argv = (char**)malloc(sizeof(char*) * (sz + 1));
        if (argv == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < sz; ++i) {
            argv[i] = strdup(wget_args[i].c_str());
            if (argv[i] == NULL) {
                perror("strdup");
                exit(EXIT_FAILURE);
            }
            if (debug_agent)
                eprintf(" %s", argv[i]);
        }
        argv[sz] = 0;
        if (debug_agent)
            eprintf("\n");

    retry_dup:
        /* Dup stdout onto our pipe */

        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            if (errno == EINTR)
                goto retry_dup;
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        close(pipefd[0]);

        execvp("wget", argv);
        perror("execvp: wget");
        exit(EXIT_FAILURE);
    }

    /* parent */

    close(pipefd[1]);

    /* Read until eol */

    repeat = 1;
    while (repeat) {
        bread = read(pipefd[0], buffer, CHUNK_SZ);
        if (bread == -1) {
            if (errno == EINTR)
                continue;
            else {
                perror("read");
                repeat = 0;
                rv = 0;
            }
        } else if (bread == 0) {
            repeat = 0;
        } else {
            sresponse.append(buffer, bread);
        }
    }

    /* This is a blocking wait */
retry_wait:
    if (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR)
            goto retry_wait;
        else {
            perror("waitpid");
            rv = 0;
        }
    }

    unlink(tmpfile);
    if (!rv)
        return 0;

    if (WIFEXITED(status)) {
        int exitcode = WEXITSTATUS(status);

        if (exitcode == WGET_AUTH_ERROR) {
            response.statusCode = IAS_UNAUTHORIZED;
        }

        else if (exitcode == WGET_NO_ERROR || exitcode == WGET_SERVER_ERROR) {
            HttpResponseParser::ParseResult result;

            result = parser.parse(response, sresponse.c_str(),
                sresponse.c_str() + sresponse.length());
            rv = (result == HttpResponseParser::ParsingCompleted);
        }
    } else
        rv = 0;

    return rv;
}
