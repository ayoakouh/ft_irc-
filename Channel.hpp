#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <vector>
#include <sstream>
#include <map>

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
		size_t		channel_size;
        std::string topic; // TOPIC
        bool topic_restricted; // TOPIC
        std::map<std::string, bool> channel_members;
        std::string topic_setter; //THE CLIENT THAT LAST SET THE TOPIC
        std::time_t timestamp_for_last_topic_set; //the last time a user set a TOPIC
    public:
        Channel(void);
		Channel(std::string &channel_name);
        ~Channel(void);
        Channel(const Channel &obj);
        Channel &operator=(const Channel &obj);
        void	add(int fd);
        void	pop(int fd);
        bool	check_member(int fd);
        void	become_op(int fd); // +o
        void	pop_op(int fd); // -o
        bool	check_op(int fd);
		void	add_invite(int fd);
		void	pop_invite(int fd);
		bool	check_invite(int fd);
		const std::string &get_name(void);
		std::vector<int> &get_members(void);
		bool get_invite_only(void);
        size_t get_channel_size(void);
        std::string getTopic(); // TOPIC
        // void setTopic(const std::string &new_topic); // TOPIC
        bool isTopicRestricted(); // TOPIC
        void set_Topic_Restricted(bool status); // TOPIC
		bool check_key(void);// anass you implement this
		std::string &get_key(void); // and this
        void set_invite_only(bool status_of_invite_only);
        void set_key(const std::string &new_key);
        void remove_key();
        void set_bool_key();//this one added for setting the is_key bool
        void                        set_channel_members(std::string &name, bool b);
        std::map<std::string, bool> &get_channel_members(void);


        void set_channel_size(int new_size); // l-/l+
        std::string &get_topic_setter(void);
        std::string get_timestamp(void);
        void        setTopic(const std::string &new_topic, const std::string &new_topic_setter);

};


#endif