#include <iostream>
#include "main.h"

uint64_t localPlayerOffset = 0x514AA08;
uint64_t playerOffset = 0x50C7588;
uint64_t glowInfoOffset = 0x595A2d0;

uint64_t teamOffset = 0x128;
uint64_t healthOffset = 0x134;
uint64_t flagOffset = 0x138;
uint64_t crosshairIDOffset = 0xB380;
uint64_t glowIndexOffset = 0xAC00;

uint64_t DT_BasePlayerOffset = 0xB46B9A;
uint64_t currentWeaponOffset = 0xCF02DE4;

mach_vm_address_t clientModule;
mach_vm_address_t engineModule;

bool statBool = true;

bool hackEnabled = true;

bool canGlow = true;
bool canHop = true;
bool canTrigger = true;

bool redrawInterface = true;

bool noPid = false;
bool noTask = false;

struct Color
{
    float red;
    float green;
    float blue;
    float alpha;
};


int get_process()
{
    pid_t pids[1024];
    int numberOfProcesses = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
    proc_listpids(PROC_ALL_PIDS, 0, pids, sizeof(pids));
    for (int i = 0; i < numberOfProcesses; ++i)
    {
        if (pids[i] == 0) { continue; }
        char name[1024];
        proc_name(pids[i], name, sizeof(name));
        if (!strncmp(name, "csgo_osx64", sizeof("csgo_osx64")))
        {
            return pids[i];
        }
    }
    return -1;
}

int get_client_module_info(task_t task, pid_t pid, mach_vm_address_t * start, unsigned long * length)
{
    task_for_pid(current_task(), pid, &task);
    struct task_dyld_info dyld_info;
    mach_vm_address_t address;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    if (task_info(task, TASK_DYLD_INFO, (task_info_t)&dyld_info, &count) == KERN_SUCCESS)
    {
        address = dyld_info.all_image_info_addr;
    }
    struct dyld_all_image_infos *dyldaii;
    mach_msg_type_number_t size = sizeof(dyld_all_image_infos);
    vm_offset_t readMem;
    kern_return_t kr = vm_read(task, address, size, &readMem, &size);
    if (kr != KERN_SUCCESS)
    {
        return 0;
    }
    dyldaii = (dyld_all_image_infos *) readMem;
    int imageCount = dyldaii->infoArrayCount;
    mach_msg_type_number_t dataCnt = imageCount * 24;
    struct dyld_image_info *g_dii = NULL;
    g_dii = (struct dyld_image_info *) malloc (dataCnt);
    // 32bit bs 64bit
    kern_return_t kr2 = vm_read(task, (mach_vm_address_t)dyldaii->infoArray, dataCnt, &readMem, &dataCnt);
    if (kr2)
    {
        return 0;
    }
    else
    {
        
    }
    struct dyld_image_info *dii = (struct dyld_image_info *) readMem;
    for (int i = 0; i < imageCount; i++)
    {
        dataCnt = 1024;
        vm_read(task, (mach_vm_address_t)dii[i].imageFilePath, dataCnt, &readMem, &dataCnt);
        char *imageName = (char *)readMem;
        if (imageName)
        {
            g_dii[i].imageFilePath = strdup(imageName);
        }
        else
        {
            g_dii[i].imageFilePath = NULL;
        }
        g_dii[i].imageLoadAddress = dii[i].imageLoadAddress;
        if (strstr(imageName, "/client.dylib") != NULL )
        {
            clientModule = (mach_vm_address_t)dii[i].imageLoadAddress;
            *start = (mach_vm_address_t)dii[i].imageLoadAddress;
        }
        if (strstr(imageName, "/engine.dylib") != NULL )
        {
            engineModule = (mach_vm_address_t)dii[i].imageLoadAddress;
            *start = (mach_vm_address_t)dii[i].imageLoadAddress;
        }
    }
    return task;
}

int getEntityGlowLoopStartAddress(task_t task, mach_vm_address_t imgbase, uint64_t glowInfoOffset, uint64_t * address)
{
    auto reAddress = Utils::ReadMemAndDeAllocate<uint64_t>(task, current_task() ,imgbase + glowInfoOffset, address);
    
    if (!reAddress)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


struct moduleInfo_t
{
    pid_t pid;
    task_t task = 0;
    mach_vm_address_t moduleStartAddress;
    unsigned long moduleLength = 0x0511e000;
    uint64_t glowObjectLoopStartAddress;
    
    void readInformation()
    {
        pid = get_process();
        task = get_client_module_info(task, pid, &moduleStartAddress, &moduleLength);
        getEntityGlowLoopStartAddress(task, moduleStartAddress, glowInfoOffset, &glowObjectLoopStartAddress);
    }
}moduleInfo;

struct myPlayer_t
{
    uint64_t    playerBase;
    int         teamNum;
    int         health;
    int         state;
    int         chID;
    int         weapon;
    
    void readInformation()
    {
        Utils::ReadMemAndDeAllocate(moduleInfo.task,
                                    current_task(),
                                    moduleInfo.moduleStartAddress + localPlayerOffset,
                                    &playerBase);
        
        Utils::ReadMemAndDeAllocate(moduleInfo.task,
                                    current_task(),
                                    playerBase + teamOffset,
                                    &teamNum);
        
        Utils::ReadMemAndDeAllocate(moduleInfo.task,
                                    current_task(),
                                    playerBase + healthOffset,
                                    &health);
        
        Utils::ReadMemAndDeAllocate(moduleInfo.task,
                                    current_task(),
                                    playerBase + flagOffset,
                                    &state);
        
        Utils::ReadMemAndDeAllocate(moduleInfo.task,
                                    current_task(),
                                    playerBase + crosshairIDOffset,
                                    &chID);
        
        Utils::ReadMemAndDeAllocate(moduleInfo.task,
                                    current_task(),
                                    playerBase - currentWeaponOffset,
                                    &weapon);
    }
}myPlayer;

struct enemyList_t
{
    uint64_t    enemyBase;
    int         teamNum;
    int         health;
    int         glowIndex;
    
    void readInformation(int currentPlayer)
    {
        Utils::ReadMemAndDeAllocate<uint64_t>(moduleInfo.task,
                                    current_task(),
                                    moduleInfo.moduleStartAddress + playerOffset + 0x20 * currentPlayer,
                                    &enemyBase);
        
        Utils::ReadMemAndDeAllocate<int>(moduleInfo.task,
                                         current_task(),
                                         enemyBase + teamOffset,
                                         &teamNum);
        
        Utils::ReadMemAndDeAllocate<int>(moduleInfo.task,
                                         current_task(),
                                         enemyBase + healthOffset,
                                         &health);
        
        Utils::ReadMemAndDeAllocate<int>(moduleInfo.task,
                                         current_task(),
                                         enemyBase + glowIndexOffset,
                                         &glowIndex);
    }
}enemyList[60];

Boolean isPressed(unsigned short inKeyCode)
{
    unsigned char keyMap[16];
    GetKeys((BigEndianUInt32*) &keyMap);
    return (0 != ((keyMap[ inKeyCode >> 3] >> (inKeyCode & 7)) & 1));
}

CGEventRef keyBoardCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    if ((type != kCGEventKeyDown) && (type != kCGEventKeyUp) && (type != kCGEventFlagsChanged))
        return event;
    
    CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    if(keycode == kVK_Control)
    {
        CGEventSetFlags(event,(CGEventFlags)(NX_COMMANDMASK|CGEventGetFlags(event)));
    }
    CGEventSetIntegerValueField(event, kCGKeyboardEventKeycode, (int64_t)keycode);
    if(isPressed(kVK_Control))
    {
        if(keycode == kVK_ANSI_M)
        {
            redrawInterface = true;
            hackEnabled = !hackEnabled;
        }
        
        if(keycode == kVK_ANSI_J)
        {
            redrawInterface = true;
            canGlow = !canGlow;
        }
        
        if(keycode == kVK_ANSI_K)
        {
            redrawInterface = true;
            canHop = !canHop;
        }
        
        if(keycode == kVK_ANSI_L)
        {
            redrawInterface = true;
            canTrigger = !canTrigger;
        }
    }
    
    /**
     Shutdown BHop if you type a message in chat
     */
    if(canHop && (keycode == kVK_ANSI_Y || keycode == kVK_ANSI_U))
    {
        canHop = !canHop;
    }
    
    if(!canHop && keycode == kVK_Return)
    {
        canHop = !canHop;
    }
    
    return event;
}

void keyBoardListen(){
    CFMachPortRef       eventTap;
    CGEventMask         eventMask;
    CFRunLoopSourceRef  runLoopSource;
    
    eventMask = ((1 << kCGEventKeyDown) | (1 << kCGEventFlagsChanged));
    
    eventTap = CGEventTapCreate((CGEventTapLocation)kCGSessionEventTap, (CGEventTapPlacement)kCGHeadInsertEventTap, (CGEventTapOptions)0, eventMask, keyBoardCallback, NULL);
    if (!eventTap)
    {
        exit(1);
    }
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    CFRunLoopRun();
}

template <typename Type>
void WriteMem(task_t task, mach_vm_address_t address, Type value)
{
    vm_write(task, address, (vm_offset_t) &value, sizeof(Type));
}

void applyGlowEffect(int glowObjectIndex,
                     Color * color)
{
    WriteMem<bool>(moduleInfo.task, moduleInfo.glowObjectLoopStartAddress + 0x40 * glowObjectIndex + 0x28, statBool);
    WriteMem<float>(moduleInfo.task, moduleInfo.glowObjectLoopStartAddress + 0x40 * glowObjectIndex + 0x8, color->red);
    WriteMem<float>(moduleInfo.task, moduleInfo.glowObjectLoopStartAddress + 0x40 * glowObjectIndex + 0xc, color->green);
    WriteMem<float>(moduleInfo.task, moduleInfo.glowObjectLoopStartAddress + 0x40 * glowObjectIndex + 0x10, color->blue);
    WriteMem<float>(moduleInfo.task, moduleInfo.glowObjectLoopStartAddress + 0x40 * glowObjectIndex + 0x14, color->alpha);
}

void sendKey(CGKeyCode keyCode)
{
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
    
    CGEventRef spaceKeyUp;
    CGEventRef spaceKeyDown;
    
    spaceKeyUp = CGEventCreateKeyboardEvent(source, keyCode, false);
    spaceKeyDown = CGEventCreateKeyboardEvent(source, keyCode, true);
    
    CGEventPost(kCGHIDEventTap, spaceKeyDown);
    CGEventPost(kCGHIDEventTap, spaceKeyUp);
    
    CFRelease(spaceKeyUp);
    CFRelease(spaceKeyDown);
    CFRelease(source);
}

void triggerShoot()
{
    CGEventRef event = CGEventCreate(NULL);
    CGPoint cursor = CGEventGetLocation(event);
    CFRelease(event);
    
    CGEventRef click_down = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, cursor, kCGMouseButtonLeft);
    CGEventRef click_up = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, cursor, kCGMouseButtonLeft);
    
    usleep(30000);
    CGEventPost(kCGHIDEventTap, click_down);
    usleep(30000);
    CGEventPost(kCGHIDEventTap, click_up);
    usleep(30000);

    CFRelease(click_down);
    CFRelease(click_up);
}

void applyGlow(enemyList_t enemy)
{
    if (enemy.teamNum == 0)
    {
        return;
    }
    
    if (enemy.health == 0)
    {
        return;
    }
        
    float red = 1.0f;
    float blue = 1.0f;
    float green = 0.0f;
    float alpha = 0.7f;
    if (enemy.teamNum == 3)
    {
        red = 0.0f;
        green = 1.0f;
        blue = 1.0f;
    }
    
    if (enemy.health < 100 && enemy.health > 0 && enemy.teamNum != myPlayer.teamNum)
    {
        red = float((100 - enemy.health) / 100.0);
        green = float((enemy.health) / 100.0);
        blue = 0.0f;
    }
    Color color = {red, green, blue, alpha};
        
    applyGlowEffect(enemy.glowIndex, &color);
}

void triggerOnEnemyTeam()
{
    if(myPlayer.chID == 0)
    {
        return;
    }
    if(myPlayer.chID > 60)
    {
        return;
    }
    if(enemyList[myPlayer.chID - 1].teamNum == myPlayer.teamNum)
    {
        return;
    }
    triggerShoot();
}

void bHop()
{
    if(
       isPressed(kVK_Space)
       &&
       (
            myPlayer.state == 257
        ||
            myPlayer.state == 263
       )
       &&
       myPlayer.health > 0
    )
    {
        sendKey(kVK_Space);
    }
}

void printInterface(void)
{
    std::system("clear");
    printf("|///////////////////////////////////////////////////////////////////////|\n");
    printf("|                           CS:GO GOSX Multihack                        |\n");
    printf("|///////////////////////////////////////////////////////////////////////|\n");
    if(noPid)
    {
        printf("|                                                                       |\n");
        printf("| Counterstrike: Global Offensive is not running!                       |\n");
        printf("|                                                                       |\n");
        printf("|///////////////////////////////////////////////////////////////////////|\n");
    }
    else if(noTask)
    {
        printf("|                                                                       |\n");
        printf("| Could not find the task for Counterstrike: Global Offensive           |\n");
        printf("|                                                                       |\n");
        printf("|///////////////////////////////////////////////////////////////////////|\n");
    }
    else
    {
        printf("|                    |            |                       |             |\n");
        printf("| Status (CTRL + M)  | ");
        if(hackEnabled)
        {
            printf("enabled    ");
        }
        else
        {
            printf("disabled   ");
        }
        printf("| Bunnyhop (CTRL + K)   | ");
        if (hackEnabled && canHop)
        {
            printf("active      ");
        }
        else
        {
            printf("not active  ");
        }
        printf("|\n");
        printf("|                    |            |                       |             |\n");
        printf("|////////////////////|////////////|///////////////////////|/////////////|\n");
        printf("|                                 |                       |             |\n");
        printf("|                                 | Wallhack (CTRL + J)   | ");
        if (hackEnabled && canGlow)
        {
            printf("active      ");
        }
        else
        {
            printf("not active  ");
        }
        printf("|\n");
        printf("|                                 |                       |             |\n");
        printf("|                                 |///////////////////////|/////////////|\n");
        printf("|                                 |                       |             |\n");
        printf("|                                 | Triggerbot (CTRL + L) | ");
        if (hackEnabled && canTrigger)
        {
            printf("active      ");
        }
        else
        {
            printf("not active  ");
        }
        printf("|\n");
        printf("|                                 |                       |             |\n");
        printf("|/////////////////////////////////|///////////////////////|/////////////|\n");
    }
}

int main(int argc, const char * argv[])
{
    moduleInfo.readInformation();
    
    if (moduleInfo.pid == -1)
    {
        noPid = true;
        printInterface();
        exit(1);
    }
    
    if (moduleInfo.task == 0)
    {
        noTask = true;
        printInterface();
        exit(1);
    }
    
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
        keyBoardListen();
    });

    while (true)
    {
        if(hackEnabled)
        {
            myPlayer.readInformation();
            int playersOnServer = 0;
            for (int i = 0; i < 60; i++)
            {
                enemyList[i].readInformation(i);
                if(enemyList[i].enemyBase != 0x0)
                {
                    playersOnServer++;
                }
            }
        
            if (canHop)
            {
                bHop();
            }
            if (canGlow)
            {
                for (int i = 0; i < playersOnServer; i++)
                {
                    applyGlow(enemyList[i]);
                }
            }
            if (canTrigger)
            {
                triggerOnEnemyTeam();
            }
        }
        if(redrawInterface)
        {
            printInterface();
            redrawInterface = false;
        }
        usleep(7800);
    }
    
}