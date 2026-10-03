#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl;

    SmallShell &Shell = SmallShell::getInstance();
    if( (Shell.current_running_pid != -1) && (Shell.current_running_pid != Shell.smash_pid) ){
        pid_t fgProcess = Shell.current_running_pid;
        if(kill(fgProcess, SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
        Shell.getJobs().removeJobByPID(Shell.current_running_pid);
        cout << "smash: process " << Shell.current_running_pid << " was killed" << endl;
    }
    Shell.current_running_pid = -1;

}
