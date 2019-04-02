#ifndef WATCHDOG_H
#define WATCHDOG_H

int watchdog_init(void);
void watchdog_deinit(void);
void watchdog_kick(void);

#endif // WATCHDOG_H
