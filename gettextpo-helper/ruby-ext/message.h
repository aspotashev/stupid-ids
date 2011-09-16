
#include <ruby.h>

class Message;

VALUE cMessage_wrap(Message *message);

void init_Message();

