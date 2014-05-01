/*
* Copyright (C) 2014 - plutoo
* Copyright (C) 2014 - ichfly
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "svc.h"

static const char* names[256] = {
    NULL,
    "ControlMemory",
    "QueryMemory",
    "ExitProcess",
    "GetProcessAffinityMask",
    "SetProcessAffinityMask",
    "GetProcessIdealProcessor",
    "SetProcessIdealProcessor",
    "CreateThread",
    "ExitThread",
    "SleepThread",
    "GetThreadPriority",
    "SetThreadPriority",
    "GetThreadAffinityMask",
    "SetThreadAffinityMask",
    "GetThreadIdealProcessor",
    "SetThreadIdealProcessor",
    "GetCurrentProcessorNumber",
    "Run",
    "CreateMutex",
    "ReleaseMutex",
    "CreateSemaphore",
    "ReleaseSemaphore",
    "CreateEvent",
    "SignalEvent",
    "ClearEvent",
    "CreateTimer",
    "SetTimer",
    "CancelTimer",
    "ClearTimer",
    "CreateMemoryBlock",
    "MapMemoryBlock",
    "UnmapMemoryBlock",
    "CreateAddressArbiter",
    "ArbitrateAddress",
    "CloseHandle",
    "WaitSynchronization1",
    "WaitSynchronizationN",
    "SignalAndWait",
    "DuplicateHandle",
    "GetSystemTick",
    "GetHandleInfo",
    "GetSystemInfo",
    "GetProcessInfo",
    "GetThreadInfo",
    "ConnectToPort",
    "SendSyncRequest1",
    "SendSyncRequest2",
    "SendSyncRequest3",
    "SendSyncRequest4",
    "SendSyncRequest",
    "OpenProcess",
    "OpenThread",
    "GetProcessId",
    "GetProcessIdOfThread",
    "GetThreadId",
    "GetResourceLimit",
    "GetResourceLimitLimitValues",
    "GetResourceLimitCurrentValues",
    "GetThreadContext",
    "Break",
    "OutputDebugString",
    "ControlPerformanceCounter",
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL,
    "CreatePort",
    "CreateSessionToPort",
    "CreateSession",
    "AcceptSession",
    "ReplyAndReceive1",
    "ReplyAndReceive2",
    "ReplyAndReceive3",
    "ReplyAndReceive4",
    "ReplyAndReceive",
    "BindInterrupt",
    "UnbindInterrupt",
    "InvalidateProcessDataCache",
    "StoreProcessDataCache",
    "FlushProcessDataCache",
    "StartInterProcessDma",
    "StopDma",
    "GetDmaState",
    "RestartDma",
    NULL,
    "DebugActiveProcess",
    "BreakDebugProcess",
    "TerminateDebugProcess",
    "GetProcessDebugEvent",
    "ContinueDebugEvent",
    "GetProcessList",
    "GetThreadList",
    "GetDebugThreadContext",
    "SetDebugThreadContext",
    "QueryDebugProcessMemory",
    "ReadProcessMemory",
    "WriteProcessMemory",
    "SetHardwareBreakPoint",
    "GetDebugThreadParam",
    NULL, NULL,
    "ControlProcessMemory",
    "MapProcessMemory",
    "UnmapProcessMemory",
    NULL, NULL, NULL,
    "TerminateProcess",
    NULL,
    "CreateResourceLimit",
    NULL, NULL,
    "KernelSetState",
    "QueryProcessMemory",
    NULL
};


void svc_Execute(ARMul_State * state, u8 num)
{
    const char* name = names[num & 0xFF];

    if(name == NULL)
        name = "Unknown";

    DEBUG("-- svc%s (0x%x) --\n", name, num);

    switch (num) {
    case 1:
        arm11_SetR(0, svcControlMemory());
        return;
    case 2:
        //Stubbed for now
        //arm11_SetR(0, svcQueryMemory());
        arm11_SetR(0, 0);
        return;
    case 8:
        arm11_SetR(0, svcCreateThread());
        return;
    case 9: //Exit Thread
        arm11_SetR(0, 0);
        state->NumInstrsToExecute = 0;
        threads_removecurrent();
        return;
    case 0xa:
        arm11_SetR(0, svcsleep());
        return;
    case 0x13:
        arm11_SetR(0, svcCreateMutex());
        return;
    case 0x14:
        arm11_SetR(0, svcReleaseMutex());
        return;
    case 0x17:
        arm11_SetR(0, svcCreateEvent());
        return;
    /*case 0x1E:
        arm11_SetR(0, svcMapMemoryBlock());
        return;*/
    case 0x1F:
        arm11_SetR(0, svcMapMemoryBlock());
        return;
    case 0x21:
        arm11_SetR(0, svcCreateAddressArbiter());
        return;
    case 0x22:
        arm11_SetR(0, svcArbitrateAddress());
        return;
    case 0x23:
        arm11_SetR(0, svcCloseHandle());
        return;
    case 0x24:
        arm11_SetR(0, svcWaitSynchronization1());
        state->NumInstrsToExecute = 0;
        return;
    case 0x25:
        arm11_SetR(0, svcWaitSynchronizationN());
        state->NumInstrsToExecute = 0;
        return;
    case 0x27:
        arm11_SetR(0, svcDuplicateHandle());
        return;
    case 0x28: //GetSystemTick
        arm11_SetR(0, 1);
        return;
    case 0x2D:
        arm11_SetR(0, svcConnectToPort());
        return;
    case 0x32:
        arm11_SetR(0, svcSendSyncRequest());
        return;
    case 0x38:
        DEBUG("resourcelimit=%08x, handle=%08x\n", arm11_R(0), arm11_R(1));
        DEBUG("STUBBED");
        PAUSE();
        //arm11_SetR(0, 1);
        arm11_SetR(1, handle_New(0, 0)); // r1 = handle_out
        return;
    case 0x3A:
        DEBUG("values_ptr=%08x, handleResourceLimit=%08x, names_ptr=%08x, nameCount=%d\n",
              arm11_R(0), arm11_R(1), arm11_R(2), arm11_R(3));
        DEBUG("STUBBED");
        PAUSE();
        //arm11_SetR(0, 1);
        mem_Write32(arm11_R(0), 0); //Set used memory to 0 for now
        return;
    case 0x3C: //Break
        exit(1);
        return;
    case 0x3D: //OutputDebugString
    {
        char temp[256];
        memset(temp,0,256);
        mem_Read(temp, arm11_R(0), arm11_R(1));
        DEBUG("%s\n",temp);
        //arm11_Dump();
        return;
    }
    case 0xFF:
        fprintf(stdout, "%c", (u8)arm11_R(0));
        return;

    default:
        //Lets not error yet
        arm11_SetR(0, 0);
        break;
    }

    arm11_Dump();
    PAUSE();
    //exit(1);
}
