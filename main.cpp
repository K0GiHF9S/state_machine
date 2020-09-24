#include "mail_sender.h"
#include "state_machine.h"

class A
{
    int a = 1;
};

// void* operator new(unsigned long, void*);

int main(void)
{
    constexpr auto a =  lib::internal::create_vtable<A>();
}
