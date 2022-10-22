#ifndef base_H
#define base_H

#include "define.h"
#include "struct.h"

enum ws_comm {
  OFF,
  ON,
  ERASE,
  SETTINGS,
  SAVE,
  REBOOT,
  INFO,
  PING,
  SCAN,
  PROGRESS,
  FILES,
  END
};

#endif