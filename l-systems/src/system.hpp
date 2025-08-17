#ifndef LSYSTEMS_SYSTEM_HPP
#define LSYSTEMS_SYSTEM_HPP

#include "Application.hpp"
#include "args.hpp"

void register_system(Application &application, ContextPtr &context);

void check_buffers_compatibility(int required_number_of_buffers);

#endif //LSYSTEMS_SYSTEM_HPP
