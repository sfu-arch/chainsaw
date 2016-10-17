#include "O3sim_ruby/O3sim_ruby.h"
#include <iostream>

int main()
{
    O3sim_ruby *m_ruby;
    int i = 0;

    m_ruby = new O3sim_ruby(1, 1, 16, 16, true, true, 1 ,"l","med","debug");

    m_ruby->initialize();

    m_ruby->send_request(0x400, 16, 0, 0, false, false, NULL);

    while (i < 1000)
    {
        m_ruby->advance_time();
        i++;
    }

    m_ruby->destroy();

    delete m_ruby;

    return 0;

}

