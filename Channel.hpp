#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>

class Channel
{
    private:
        std::string name;
        std::vector<int> members;
        std::vector<int> op;
    public:
        Channel(void);
        ~Channel(void);
        Channel(const Channel &obj);
        Channel &operator=(const Channel &obj);
        add(int fd);
        pop(int fd);
        check_member(int fd);
        become_op(int fd);
        pop_op(int fd);
        check_op(int fd);
};








#endif