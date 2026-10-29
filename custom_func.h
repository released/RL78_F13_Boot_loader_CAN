/* Project/API reading notes: User entry declarations shared with the selected main/startup
 * adapter.
 * Implement application initialization in hardware_init() and recurring foreground work in
 * loop(); peripheral ISR declarations remain with their owning driver.
 */
#ifndef CUSTOM_FUNC_H
#define CUSTOM_FUNC_H

/* User-owned Boot initialization and foreground session. */
void hardware_init(void);
void loop(void);

void boot_R_MAIN_UserInit(void);

#endif
