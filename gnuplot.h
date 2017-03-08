#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

class gnuplot
{
public:
    virtual ~gnuplot() {
        close();
    }

    bool is_opened() const { return m_pipe != nullptr; }

    void open() {
        close();
        m_pipe = gp_popen("gnuplot", "w");
    }

    void close() {
        if (is_opened()) {
            gp_pclose(m_pipe);
            m_pipe = nullptr;
        }
    }

    void command(const std::string &cmd) {
        if (is_opened() && !cmd.empty()) {
            fprintf(m_pipe, "%s\n", cmd.c_str());
            fflush(m_pipe);
        }
    }

private:
    FILE* m_pipe = nullptr;

    static FILE* gp_popen(const char *command, const char *mode) {
        return _popen(command, mode);
    }

    static int gp_pclose(FILE *pipe) {
        return _pclose(pipe);
    }
};
