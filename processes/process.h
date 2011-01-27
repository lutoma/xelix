#pragma once

#include <common/generic.h>
#include <interrupts/interface.h>
#include <memory/paging/paging.h>

void process_create(char name[100], void function());
