## poll
所以，实际上就是会卡到这个代码上，不会出现异步的情况

## 附录

### source code
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <time.h>

int main(int argc, char *argv[]) {
  int numPipes, ready, j;
  struct pollfd *pollFd;
  int pfds[1]; /* File descriptors for all pipes */

  /* Allocate the arrays that we use. The arrays are sized according
     to the number of pipes specified on command line */

  numPipes = 1;

  pollFd = calloc(numPipes, sizeof(struct pollfd));
  if (pollFd == NULL) {
    exit(EXIT_FAILURE);
  }

  /* Create the number of pipes specified on command line */

  for (j = 0; j < numPipes; j++)
    pfds[j] = 0;

  /* Build the file descriptor list to be supplied to poll(). This list
     is set to contain the file descriptors for the read ends of all of
     the pipes. */

  for (j = 0; j < numPipes; j++) {
    pollFd[j].fd = pfds[j];
    pollFd[j].events = POLLIN;
  }

  printf("huxueshi:%s before \n", __FUNCTION__);
  ready = poll(pollFd, numPipes, -1);
  printf("huxueshi:%s after \n", __FUNCTION__);
  if (ready == -1) {
    exit(EXIT_FAILURE);
  }

  printf("poll() returned: %d\n", ready);

  /* Check which pipes have data available for reading */

  for (j = 0; j < numPipes; j++)
    if (pollFd[j].revents & POLLIN) {
      printf("Readable: %3d\n", pollFd[j].fd);
      char x[100];
      scanf("%s", x);
      printf("[%s]", x);
    }

  exit(EXIT_SUCCESS);
}
```

### backtrace

```c
/*
#0  da_ConPoll (filp=0x7e06e018, events=1) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Uefi/Devices/Console/daConsole.c:658
#1  0x000000007e001bbd in poll (nfds=1, timeout=-1, pfd=0x7e06a038) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/BaseDebugLibNull/DebugLib.c:166
#2  main (argc=<optimized out>, argv=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/AppPkg/Applications/Main/Main.c:76
#3  ShellAppMain (Argc=<optimized out>, Argv=0x7e076d98) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Main/Main.c:182
#4  0x000000007e0052f9 in ShellCEntryLib (SystemTable=0x7f9ee018, ImageHandle=0x7e076898) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Library/UefiShellCEn
tryLib/UefiShellCEntryLib.c:84
#5  ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7e076898) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/AppPkg/DEBUG_GCC5/X64/AppPkg/App
lications/Main/Main/DEBUG/AutoGen.c:375
#6  _ModuleEntryPoint (ImageHandle=0x7e076898, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#7  0x000000007feba8b5 in CoreStartImage (ImageHandle=0x7e076898, ExitDataSize=0x0, ExitData=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe
/Image/Image.c:1653
#8  0x000000007df658b7 in InternalShellExecuteDevicePath (ParentImageHandle=0x7dfe5ad8, DevicePath=DevicePath@entry=0x7e076a98, CommandLine=CommandLine@entry=0x7e08e69
8, Environment=Environment@entry=0x0, StartImageStatus=StartImageStatus@entry=0x7fe9e718) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Sh
ellProtocol.c:1540
#9  0x000000007df68ab4 in RunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e0ca198, FirstParameter=0x7e08e198, CmdLine=0x7e08e698, Type=Efi_Application) at /home/
maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2505
#10 SetupAndRunCommandOrFile (CommandStatus=0x0, ParamProtocol=0x7e0ca198, FirstParameter=0x7e08e198, CmdLine=<optimized out>, Type=Efi_Application) at /home/maritns3/
core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2589
#11 RunShellCommand (CommandStatus=0x0, CmdLine=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2713
#12 RunShellCommand (CmdLine=<optimized out>, CommandStatus=0x0) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2625
#13 0x000000007df0c0ed in RunCommand (CmdLine=0x7e077018) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:2765
#14 RunScriptFileHandle (Name=<optimized out>, Handle=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:3026
#15 RunScriptFile (Handle=0x0, ParamProtocol=<optimized out>, CmdLine=<optimized out>, ScriptPath=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Shel
lPkg/Application/Shell/Shell.c:3130
#16 RunScriptFile.constprop.0 (ScriptPath=<optimized out>, CmdLine=<optimized out>, ParamProtocol=<optimized out>, Handle=0x0) at /home/maritns3/core/ld/edk2-workstati
on/edk2/ShellPkg/Application/Shell/Shell.c:3099
#17 0x000000007df6c15a in DoStartupScript (FilePath=0x7e0c8318, ImagePath=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/S
hell.c:1282
#18 UefiMain (ImageHandle=<optimized out>, SystemTable=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/ShellPkg/Application/Shell/Shell.c:594
#19 0x000000007df5052d in ProcessModuleEntryPointList (SystemTable=0x7f9ee018, ImageHandle=0x7f130218) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DE
BUG_GCC5/X64/ShellPkg/Application/Shell/Shell/DEBUG/AutoGen.c:1013
#20 _ModuleEntryPoint (ImageHandle=0x7f130218, SystemTable=0x7f9ee018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/UefiApplicationEntryPoint/Applica
tionEntryPoint.c:59
#21 0x000000007feba8b5 in CoreStartImage (ImageHandle=0x7f130218, ExitDataSize=0x7e1e06c8, ExitData=0x7e1e06c0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModu
lePkg/Core/Dxe/Image/Image.c:1653
#22 0x000000007f05d5e2 in EfiBootManagerBoot (BootOption=BootOption@entry=0x7e1e0678) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Library/UefiBootMana
gerLib/BmBoot.c:1982
#23 0x000000007f060ca2 in BootBootOptions (BootManagerMenu=0x7fe9ecd8, BootOptionCount=5, BootOptions=0x7e1e0518) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeMo
dulePkg/Universal/BdsDxe/BdsEntry.c:409
#24 BdsEntry (This=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/BdsDxe/BdsEntry.c:1072
#25 0x000000007feaabe3 in DxeMain (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c:551
#26 0x000000007feaac88 in ProcessModuleEntryPointList (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/Build/OvmfX64/DEBUG_GCC5/X64/MdeModule
Pkg/Core/Dxe/DxeMain/DEBUG/AutoGen.c:489
#27 _ModuleEntryPoint (HobStart=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.c:48
#28 0x000000007fee10cf in InternalSwitchStack ()
#29 0x0000000000000000 in ?? ()
*/
```

如果在正在 poll 的时候在 gdb 中 Ctrl + C，然后 bt

```c
/*
#0  KeyReadStatusRegister (ConsoleIn=ConsoleIn@entry=0x7ec76018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/BaseIoLibIntrinsic/IoLibGcc.c:50
#1  0x000000007edacc38 in KeyboardTimerHandler (Event=Event@entry=0x0, Context=Context@entry=0x7ec76018) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/B
us/Isa/Ps2KeyboardDxe/Ps2KbdCtrller.c:807
#2  0x000000007edad371 in KeyboardReadKeyStrokeWorker (ConsoleInDev=ConsoleInDev@entry=0x7ec76018, KeyData=KeyData@entry=0x7fe9e1e4) at /home/maritns3/core/ld/edk2-wor
kstation/edk2/MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KbdTextIn.c:156
#3  0x000000007edad42d in KeyboardReadKeyStroke (This=<optimized out>, Key=0x7fe9e244) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Bus/Isa/Ps2Keyboard
Dxe/Ps2KbdTextIn.c:279
#4  0x000000007ede8f56 in ConSplitterTextInPrivateReadKeyStroke (Key=0x7fe9e2ac, Private=0x7edeccc0) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Unive
rsal/Console/ConSplitterDxe/ConSplitter.c:3562
#5  ConSplitterTextInReadKeyStroke (This=<optimized out>, Key=0x7fe9e2ac) at /home/maritns3/core/ld/edk2-workstation/edk2/MdeModulePkg/Universal/Console/ConSplitterDxe
/ConSplitter.c:3623
#6  0x000000007dff9538 in da_ConRawRead (filp=filp@entry=0x7e074018, Character=Character@entry=0x7e091344 L"") at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/L
ibC/Uefi/Devices/Console/daConsole.c:257
#7  0x000000007dff9626 in da_ConPoll (filp=0x7e074018, events=<optimized out>) at /home/maritns3/core/ld/edk2-workstation/edk2/StdLib/LibC/Uefi/Devices/Console/daConso
le.c:672
#8  0x000000007e001bbd in poll (nfds=1, timeout=-1, pfd=0x7e06af38) at /home/maritns3/core/ld/edk2-workstation/edk2/MdePkg/Library/BaseDebugLibNull/DebugLib.c:166
*/
```
