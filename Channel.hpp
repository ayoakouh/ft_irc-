#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <vector>

class Channel
{
    private:
        std::string name;
        std::vector<int> members;
        std::vector<int> op;
		bool		invite_only;
		std::vector<int> inv_list;
		bool		is_key;
		std::string key;
		int			channel_size;
        Channel(void);
    public:
		Channel(std::string &channel_name);
        ~Channel(void);
        Channel(const Channel &obj);
        Channel &operator=(const Channel &obj);
        void	add(int fd);
        void	pop(int fd);
        bool	check_member(int fd);
        void	become_op(int fd);
        void	pop_op(int fd);
        bool	check_op(int fd);
		void	add_invite(int fd);
		void	pop_invite(int fd);
		bool	check_invite(int fd);
		const std::string &get_name(void);
		const std::vector<int> &get_members(void);
		bool get_invite_only(void);
		bool check_key(void);// anass you implement this
		std::string &get_key(void); // and this

};








#endif