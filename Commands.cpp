#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <regex>
#include <fcntl.h>
#include <climits>
#include <sys/stat.h>
#include <cstdio>
#include <sys/types.h>




using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#define _POSIX_C_SOURCE 200809L
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#endif


string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;
    FUNC_EXIT()
}
bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


bool isComplex(string cmdLine) {
    return (cmdLine.find('*') != string::npos) ||
           (cmdLine.find('?') != string::npos);
}


bool SigNumberIsLegal(const char *num) {
    if (num[1] == '\0') {
        return false; // Case: "-" is not a valid signal
    }
    for (int i = 1; num[i] != '\0'; i++) {
        if (!isdigit(num[i])) {
            return false;
        }
    }
    return true;
}

bool JobIdIsLegal(const char *num) {
    for (int i = 0; num[i] != '\0'; i++) {
        if (!isdigit(num[i])) {
            return false;
        }
    }
    return true;
}
JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (JobEntry *job: List) {
        if (job->getID() == jobId) {
            return job;
        }
    }
    return nullptr;
}

void JobsList::updatingAliasesInjobList(const string& alias) {
    for (const auto &job: List) {
        if (job->getCommand()->getAliasCmdLine() == alias) {
            job->getCommand()->setAlias("none");
            job->getCommand()->setCmdLine("none");
        }
    }
}

void removeAliasFromPairsVector(vector<pair<string,string>> &aliases, const string& alias)
{
    for (size_t i = 0; i < aliases.size(); ++i) {
        if (aliases[i].first == alias) {
            aliases.erase(aliases.begin() + i);
            return;
        }
    }

}

///////////////////////////////////////////////////////////////////////////////////////
Command::Command(const char *cmd_line):is_background(_isBackgroundComamnd(cmd_line)),cmdline(cmd_line),
alias("none"),alias_cmd_line(""),background_cmd_line(cmd_line){

    _removeBackgroundSign(const_cast<char*> (cmdline.c_str()));
    cmdline = _rtrim(cmdline);
    args_num = _parseCommandLine(cmdline.c_str(), args);
}

Command::~Command()
{
    for(int i = 0; i < args_num; i++){
        if (args[i]){
            free(args[i]);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////
bool aliasAlreadyExists(const vector<pair<string, string>>& pairs, const string& key){
    for (const auto& pair : pairs) {
        if (pair.first == key) {
            return true;
        }
    }
    return false;
}


bool reserved_command(const string& name) {
    if (name == "chprompt") {
        return true;
    }
    if (name == "showpid") {
        return true;
    }
    if (name == "pwd") {
        return true;
    }
    if (name == "cd") {
        return true;
    }
    if (name == "jobs") {
        return true;
    }
    if (name == "fg") {
        return true;
    }
    if (name == "quit") {
        return true;
    }
    if (name == "kill") {
        return true;
    }
    if (name == "alias") {
        return true;
    }
    if (name == "unalias") {
        return true;
    }
    if (name == "unsetenv") {
        return true;
    }
    if (name == "sysinfo") {
        return true;
    }
    if (name == "du") {
        return true;
    }
    if (name == "whoami") {
        return true;
    }
    if (name == "usbinfo") { //bonus
        return true;
    }
    if (name == ">") {
        return true;
    }
    if (name == ">>") {
        return true;
    }
    if (name == "|") {
        return true;
    }
    if (name == "|&") {
        return true;
    }
      return false;
}


//string Command::getAlias() const {}

//built in commands :)

    BuiltInCommand::BuiltInCommand(const char *cmd_line): Command(cmd_line) {}

//chprompt command
chpromptCommand::chpromptCommand(const char *cmd_line): BuiltInCommand(cmd_line)
{
    char * parts[COMMAND_MAX_ARGS] = {nullptr};
    int len = _parseCommandLine(cmd_line, parts); //check if we can use args instead
    if (len == 1)
    {
        new_prompt = "smash";
    }
    else
    {
        new_prompt = parts[1];
    }
}
void chpromptCommand::execute() {
    SmallShell::getInstance().set_prompt(new_prompt);
}

//showpid command
    ShowPidCommand::ShowPidCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
    void ShowPidCommand::execute() {
        cout << "smash pid is " << getpid() << endl;
    }
// pwd command
    GetCurrDirCommand::GetCurrDirCommand(
    const char *cmd_line): BuiltInCommand(cmd_line) {}
    void GetCurrDirCommand::execute() {
        char temp[PATH_MAX];
        char *pwd = getcwd(temp,PATH_MAX);
        if (pwd == nullptr) {
            perror("smash error: getcwd failed");
        } else
            cout << pwd << endl;
    }
//cd command
    ChangeDirCommand::ChangeDirCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
    void ChangeDirCommand::execute() {
        SmallShell &s = SmallShell::getInstance();
        string new_dir;

        if (args_num == 1) return;

        if (args_num > 2) {
            cerr << "smash error: cd: too many arguments" << endl;
            return;
        }

        char temp[PATH_MAX];
        char *pwd = getcwd(temp, PATH_MAX);
        if (pwd == nullptr) {
            perror("smash error: getcwd failed");
            return;
        }

        if (strcmp(args[1], "-") == 0) { // -
            if (s.getLastDir() == "none"){
                cerr << "smash error: cd: OLDPWD not set" << endl;
                return;
            }
            else {
                new_dir = s.getLastDir();
            }
        } else if (strcmp(args[1], "..") == 0) //..
        {
            new_dir = "..";
        } else {
            new_dir = args[1];
        }

        if (chdir(new_dir.c_str()) != 0) {
            perror("smash error: chdir failed");
            return;
        }

        s.setLastDir(temp);
    }
// jobs command
jobsCommand::jobsCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
void jobsCommand::execute() {
    SmallShell &s = SmallShell::getInstance();
    s.getJobs().printJobsList();
}

//fg command
    ForegroundCommand::ForegroundCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
    void ForegroundCommand::execute() {
        SmallShell &s = SmallShell::getInstance();
        s.getJobs().removeFinishedJobs();
        if (s.getJobs().List.empty() && args_num == 1) {
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }

        if ((args_num == 2 && !JobIdIsLegal(args[1])) || args_num > 2) {
            {
                cerr << "smash error: fg: invalid arguments" << endl;
                return;
            }
        }

        pid_t job_pid;
        if (args_num == 2) {
            int job_id = stoi(args[1]);
            if (s.getJobs().getJobById(job_id) == nullptr) {
                cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
                return;
            }

            job_pid = s.getJobs().getJobById(job_id)->getPid();
            JobsList::JobEntry *job = s.getJobs().getJobById(job_id);
            s.current_running_pid = job_pid;
            if (strcmp(job->getCommand()->getAlias().c_str(), "none") == 0) {
                cout << job->getCommand()->getCmdLine() << " " << job_pid << endl;
            } else {
                cout << job->getCommand()->getAliasCmdLine() << " " << job_pid << endl;
            }

            pid_t res = waitpid(job_pid, nullptr, WUNTRACED);
            if (res == -1) {
                s.current_running_pid = -1;
                perror("smash error: waitpid failed");
                return;
            }

            s.current_running_pid = -1;
            s.getJobs().removeJobById(job_id);

        } else //if(args_num != 2) so its = 1 anf the list isnt empty
        {
            if (s.getJobs().List.empty()) {
                cerr << "smash error: fg: jobs list is empty" << endl;
                return;
            }

            job_pid = s.getJobs().getLastJob()->getPid();
            int last_id = s.getJobs().getLastJob()->getID();
            JobsList::JobEntry *last_job = s.getJobs().getJobById(last_id);
            s.current_running_pid = job_pid;
            if (strcmp(last_job->getCommand()->getAlias().c_str(), "none") == 0) {
                cout << last_job->getCommand()->getCmdLine() << " " << job_pid << endl;
            } else {
                cout << last_job->getCommand()->getAliasCmdLine() << " " << job_pid << endl;
            }

            pid_t res = waitpid(job_pid, nullptr, WUNTRACED);
            if (res == -1) {
                s.current_running_pid = -1;
                perror("smash error: waitpid failed");
                return;
            }

            s.current_running_pid = -1;
            s.getJobs().removeJobById(last_id);
        }
    }
//quit command
    QuitCommand::QuitCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
    void QuitCommand::execute() {
        SmallShell &s = SmallShell::getInstance();
        if (args_num < 2) {
            exit(0);
        } else {
            if (strcmp(args[1], "kill")) {
                exit(0);
            } else {
                int list_size = s.getJobs().List.size();
                cout << "smash: sending SIGKILL signal to " << list_size << " jobs:" << endl;
                for (auto it = s.getJobs().List.begin(); it != s.getJobs().List.end(); ++it)
                { //print first
                    auto job = *it;
                    auto *cmd = job->getCommand();
                    cout << job->getPid() << ": " << ((strcmp(cmd->getAlias().c_str(), "none") == 0) ?
                                                      cmd->getCmdLine() : cmd->getAliasCmdLine()) << endl;
                }
                //then kill all jobs (and delete them)
                s.getJobs().killAllJobs();
            }
        }
        exit(0);
    }
//kill command
    KillCommand::KillCommand(
    const char *cmd_line): BuiltInCommand(cmd_line) {}
    void KillCommand::execute() {
        if (args_num != 3) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        if (args[1][0] != '-') {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        if (args[2] == nullptr) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        if (!JobIdIsLegal(args[2])) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        if (!SigNumberIsLegal(args[1])) //we need to check if its between 1-31?
        {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        SmallShell &s = SmallShell::getInstance();
        s.getJobs().removeFinishedJobs();
        int job_id = stoi(args[2]);
        JobsList::JobEntry *j = s.getJobs().getJobById(job_id);
        if (j != nullptr) {
            int sig = stoi(args[1] + 1);
            cout << "signal number " << sig << " was sent to pid " << j->getPid() << endl;
            if (kill(j->getPid(), sig) == -1) {
                perror("smash error: kill failed");
                return;
            }
        } else {
            cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
            return;
        }
    }
// alias command
    AliasCommand::AliasCommand(
    const char *cmd_line): BuiltInCommand(cmd_line) {}
    void AliasCommand::execute() {
        SmallShell &s = SmallShell::getInstance();
        if (args_num == 1) {
            for (const auto &pair: s.get_aliases()) {
                cout << pair.first << "='" << pair.second << "'" << endl;
            }
        }
        else
        {
            regex alias_format("^alias [a-zA-Z0-9_]+='[^']*'$");
            bool is_right_format = regex_match(cmdline, alias_format);
            if (!is_right_format) {
                cerr << "smash error: alias: invalid alias format" << endl;
                return;
            }
            string first = "alias";
            string p = cmdline.substr(first.length());
            int i = p.find('=');
            string alias = _ltrim(p.substr(0, i));
            string origin_cmd;
            origin_cmd = p.substr(i + 2, p.length() - i - 3);
            if (!aliasAlreadyExists(s.get_aliases(), alias) && !reserved_command(alias)) {
                s.get_aliases().push_back({alias, origin_cmd});
            } else {
                cerr << "smash error: alias: " << alias << " already exists or is a reserved command" << endl;
                return;
            }
        }
    }

//unalias command
UnAliasCommand::UnAliasCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
    void UnAliasCommand::execute() {
        SmallShell &s = SmallShell::getInstance();
        if(args_num != 1)
        {
            int i = 1;
            while (i < args_num) {
                string alias = string(args[i]);
                if (!aliasAlreadyExists(s.get_aliases(), alias)) {
                    cerr << "smash error: unalias: " << alias << " alias does not exist" << endl;
                    return;
                }
                s.getJobs().updatingAliasesInjobList(alias);
                removeAliasFromPairsVector(s.get_aliases(), alias.c_str());
                ++i;
            }
        }
        else
        {
            cerr << "smash error: unalias: not enough arguments" << endl;
            return;
        }
    }

//reading file into buffer
int read_file_content(const char* path, char* buf, int max_size) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return -1;
    }
    int total = 0;
    while (total < max_size - 1) {
        int res = read(fd, buf + total, max_size - 1 - total);
        if (res < 0) {
            close(fd);
            return -1;
        }
        if (res == 0) {
            break;
        }
        total += res;
    }
    close(fd);
    buf[total] = '\0';
    return total;
}

//unsetenv command
UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
void UnSetEnvCommand::execute() {
    if (args_num == 1) {
        cerr << "smash error: unsetenv: not enough arguments" << endl;
        return;
    }
    pid_t pid = getpid();
    char buffer[86420];
    string path = "/proc/" + to_string(pid) + "/environ";
    int result = read_file_content(path.c_str(), buffer, sizeof(buffer));

    if (result == -1) {
        perror("smash error: open failed");
        return;
    }
    vector<string> env_list;
    for (char* p = buffer; p < buffer + result; ) {
        string entry(p);
        env_list.push_back(entry);
        p += entry.size() + 1;
    }

    for (int i = 1; i < args_num; ++i) {//unsetting variables
        string var = args[i];
        bool exists = false;
        for (const string& left : env_list) {
            size_t end = left.find('=');
            if (left.substr(0, end) == var) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            cerr << "smash error: unsetenv: " << var << " does not exist" << endl;
            return;
        }
        for (char** env = __environ; *env != nullptr; ++env) {
            if (strncmp(*env, var.c_str(), var.size()) == 0 &&
                (*env)[var.size()] == '=') {
                char** next = env;
                while (*next != nullptr) {
                    *next = *(next + 1);
                    ++next;
                }
                break;
            }
        }
    }
}

// sysinfo command
SysInfoCommand::SysInfoCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}
void SysInfoCommand::execute() {
    char os_type[256] = {0};
    char hostname[256] = {0};
    char os_release[256] = {0};
    char time_buffer[256] = {0};
    long boot_time = 0;
    int n;
    n = read_file_content("/proc/sys/kernel/ostype", os_type, 256);
    if (n > 0 && os_type[n-1] == '\n') os_type[n-1] = '\0';//remove the newline if it exists

    n = read_file_content("/proc/sys/kernel/hostname", hostname, 256);
    if (n > 0 && hostname[n-1] == '\n') hostname[n-1] = '\0';

    n = read_file_content("/proc/sys/kernel/osrelease", os_release, 256);
    if (n > 0 && os_release[n-1] == '\n') os_release[n-1] = '\0';

    char stat_buf[4096];
    if (read_file_content("/proc/stat", stat_buf, 4096) > 0) {
        char* found = strstr(stat_buf, "btime ");
        if (found != NULL) {
            sscanf(found, "btime %ld", &boot_time);
        }
    }
    if (boot_time != 0) {
        time_t t = (time_t)boot_time;
        struct tm *tm_info = localtime(&t);
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strcpy(time_buffer, "N/A");
    }

    cout << "System: " << os_type << endl;
    cout << "Hostname: " << hostname << endl;
    cout << "Kernel: " << os_release << endl;
    cout << "Architecture: x86_64" << endl;
    cout << "Boot Time: " << time_buffer << endl;
}

//////////////////////////////////////////////////////////////////////////////
// External commands :)
ExternalCommand::ExternalCommand(const char *cmd_line): Command(cmd_line) {}
void ExternalCommand::execute() {
    SmallShell &s = SmallShell::getInstance();
    s.getJobs().removeFinishedJobs();
    pid_t pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        exit(1);
    } else if (pid > 0) {
        if (!isbackground()) //foreground
        {
            s.current_running_pid = pid;
            if (waitpid(pid, nullptr, WUNTRACED) == -1) {
                s.current_running_pid = -1;
                perror("smash error: waitpid failed");
                return;
            } else {
                s.current_running_pid = -1;
            }
        } else //background
        {
            ExternalCommand *new_job = new ExternalCommand(getCmdLine().c_str()); //check later
            new_job->setAlias(alias);
            new_job->setAliasCmdLine(alias_cmd_line);
            s.getJobs().addJob(new_job, pid);
        }
    } else // pid == 0
    {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(1);
        }

        const char *exec_path;
        const char **exec_args;

        if (isComplex(cmdline)) { //is complex
            const char *bashcommandargs[4] = {"/bin/bash", "-c", (cmdline.c_str()), nullptr};
            exec_path = bashcommandargs[0];
            exec_args = bashcommandargs;

            if (execv(exec_path, const_cast<char **>(exec_args)) == -1) {
                perror("smash error: execv failed");
                exit(1);
            }
        }
        else //is simple
        {
            if (execvp(args[0], args) == -1) {
                perror("smash error: execvp failed");
                exit(1);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////
//Special commands :)
//I/O redirection
RedirectionCommand::RedirectionCommand(const char* cmd_line, const string& type): Command(cmd_line)
{
    command_type = type;
    string trimmedCmd = _trim(string(cmd_line));
    size_t start_of_file_index = 0;
    size_t end_of_command_index = trimmedCmd.find(">>"); //search for >>
    bool isDoubleArrow = (end_of_command_index != string::npos);

    if (!isDoubleArrow) { // if >
        end_of_command_index = trimmedCmd.find('>');
    }

    if (end_of_command_index == string::npos)
        return;

    start_of_file_index = end_of_command_index + (isDoubleArrow ? 2 : 1);
    command = _trim(trimmedCmd.substr(0,end_of_command_index));
    File = _trim(trimmedCmd.substr(start_of_file_index));

    if (!File.empty()) {
        char* temp_file = new char[File.size() + 1];
        copy(File.begin(), File.end(), temp_file);
        temp_file[File.size()] = '\0';

        // remove &
        _removeBackgroundSign(temp_file);

        File = temp_file;

        delete[] temp_file;

        File = _trim(File);
    }
}

void RedirectionCommand::execute() {

    int saved_stdout = dup(1);
    if (saved_stdout == -1) {
        perror("smash error: dup failed");
        return;
    }
    int flags = O_WRONLY | O_CREAT;
    if (command_type == ">>")
        flags |= O_APPEND;
    else
        flags |= O_TRUNC;

    int fd = open(File.c_str(), flags, 0666);
    if (fd == -1) {
        perror("smash error: open failed");
        close(saved_stdout);
        return;
    }

    if (dup2(fd, 1) == -1) {
        perror("smash error: dup2 failed");
        close(fd);
        close(saved_stdout);
        return;
    }
    SmallShell& shell = SmallShell::getInstance();
    shell.executeCommand(command.c_str());

    if (close(fd) == -1) {
        perror("smash error: close failed");
    }

    if (dup2(saved_stdout, 1) == -1) {
        perror("smash error: dup2 failed");
    }

    if (close(saved_stdout) == -1) {
        perror("smash error: close failed");
    }
}
//Pipes
PipeCommand::PipeCommand(const char *cmd_line, int pipeType)
        : Command(cmd_line), pipeType(pipeType) {
    string separator = (pipeType == 1) ? "|&" : "|";
    size_t pipePos = getCmdLine().find(separator);
    if (pipePos != string::npos) {
        first_cmd = _trim(getCmdLine().substr(0, pipePos));
        second_cmd = _trim(getCmdLine().substr(pipePos + separator.length()));
        }
}

void PipeCommand::execute() {
    SmallShell &shell = SmallShell::getInstance();
    const int READ_END = 0;
    const int WRITE_END = 1;

    // create the pipe
    int fd[2];
    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    // fork the first child, the one who writes
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("smash error: fork failed");
        close(fd[READ_END]);
        close(fd[WRITE_END]);
        return;
    }

    if (pid1 == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(1);
        }

        // checking if we are piping stderr (|&) or just stdout (|)
        int sourceFD = (pipeType == 1) ? STDERR_FILENO : STDOUT_FILENO;

        // redirect output to the write end of the pipe
        if (dup2(fd[WRITE_END], sourceFD) == -1) {
            perror("smash error: dup2 failed");
            exit(1);
        }

        // close pipe fds in child
        close(fd[READ_END]);
        close(fd[WRITE_END]);

        // executing the first command
        shell.executeCommand(first_cmd.c_str());
        exit(0);
    }

    // fork the second child (the one who reads)
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("smash error: fork failed");
        close(fd[READ_END]);
        close(fd[WRITE_END]);
        return;
    }

    if (pid2 == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(1);
        }

        // redirect stdin to come from the read end of the pipe
        if (dup2(fd[READ_END], STDIN_FILENO) == -1) {
            perror("smash error: dup2 failed");
            exit(1);
        }

        // close pipe fds in child
        close(fd[READ_END]);
        close(fd[WRITE_END]);

        // executing the second command
        shell.executeCommand(second_cmd.c_str());
        exit(0);
    }

    close(fd[READ_END]);
    close(fd[WRITE_END]);

    // wait for both commands to finish
    if (waitpid(pid1, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
    }

    if (waitpid(pid2, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
    }
}

//du command
struct linux_dirent64 {
    unsigned long  d_ino;
    long           d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char       d_name[];
};


DiskUsageCommand::DiskUsageCommand(const char *cmd_line): Command(cmd_line){}
long long get_recursive_size(string path) {
    struct stat sb;
    // checking if lstat works
    if (lstat(path.c_str(), &sb) == -1) {
        return 0;
    }

    // ignore symlinks
    if (S_ISLNK(sb.st_mode)) {
        return 0;
    }

    // starting with the size of the current dir itself
    long long total = sb.st_blocks * 512;//in bytes
    if (S_ISDIR(sb.st_mode)) {
        int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
        if (fd == -1) {
            perror("smash error: open failed");
            return total;
        }


        char buf[4096];
        while (true) {
            int nread = syscall(SYS_getdents64, fd, buf, 4096);

            if (nread < 0) {
                perror("smash error: getdents failed");
                close(fd);
                return total;
            }

            if (nread == 0) {
                break;
            }

            for (int i = 0; i < nread; ) {
                struct linux_dirent64* d = (struct linux_dirent64*)(buf + i);
                string name = d->d_name;

                // skip current and parent folder links
                if (name != "." && name != "..") {
                    string next_path = path + "/" + name;
                    total += get_recursive_size(next_path);
                }
                i += d->d_reclen;
            }
        }
        close(fd);
    }
    return total;
}
void DiskUsageCommand::execute() {
    if (args_num > 2) {
        cerr << "smash error: du: too many arguments" << endl;
        return;
    }
    string searchPath = (args_num == 2) ? args[1] : ".";
    size_t bytesUsed = get_recursive_size(searchPath);
    size_t kilobytes = (bytesUsed + 1023) / 1024;  // convert to KB
    cout << "Total disk usage: " << kilobytes << " KB" << endl;
}
// whoami command
WhoAmICommand::WhoAmICommand(const char *cmd_line): Command(cmd_line) {}
void WhoAmICommand::execute() {
    uid_t uid = getuid();
    gid_t gid = getgid();

    int fd = open("/etc/passwd", O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    char buf[4096];
    string content = "";
    int n;

    while ((n = read(fd, buf, 4095)) > 0) {
        buf[n] = '\0';
        content += buf; //append the data
    }
    close(fd);

    if (n == -1) {
        perror("smash error: read failed");
        return;
    }

    string username = "";
    string home_dir = "";

    size_t start_pos = 0;
    size_t end_pos = 0;

    while ((end_pos = content.find('\n', start_pos)) != string::npos) {
        string line = content.substr(start_pos, end_pos - start_pos);
        start_pos = end_pos + 1; //nrxt line

        // now we parse the line: user:x:uid:gid:info:home:shell
        size_t c1 = line.find(':');
        if (c1 == string::npos) continue; // invalid line
        string user_field = line.substr(0, c1);

        // find UID (between 2nd and 3rd :)
        size_t c2 = line.find(':', c1 + 1); // skip 'x'
        if (c2 == string::npos) continue;
        size_t c3 = line.find(':', c2 + 1);
        if (c3 == string::npos) continue;

        string uid_field = line.substr(c2 + 1, c3 - c2 - 1);


         if(static_cast<uid_t>(atoi(uid_field.c_str())) == uid) {
            username = user_field;

            // home (field 5)
            size_t c4 = line.find(':', c3 + 1); // gid end
            size_t c5 = line.find(':', c4 + 1); // info end
            if (c5 == string::npos) break;

            size_t c6 = line.find(':', c5 + 1); // home end
            if (c6 == string::npos) c6 = line.length(); //  if line ends there

            home_dir = line.substr(c5 + 1, c6 - c5 - 1);
            break;
        }
    }

    if (username.empty()) {
        cerr << "smash error: whoami: User not found for UID " << uid << endl;
        return;
    }

    cout << username << endl;
    cout << uid << endl;
    cout << gid << endl;
    cout << home_dir << endl;
}
//usbinfo command
struct usb_device {
    int id;
    string vendor;
    string product;
    string manuf;
    string prod_name;
    string power;
};

USBInfoCommand::USBInfoCommand(const char *cmd_line): Command(cmd_line) {}
void USBInfoCommand::execute() {
    int fd = open("/sys/bus/usb/devices", O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        perror("smash error: usbinfo: no USB devices found");
        return;
    }

    char buf[4096];
    char file_content[256]; // for reading small files
    vector<usb_device> list;

    while (true) {
        int nread = syscall(SYS_getdents64, fd, buf, 4096);
        if (nread < 0) {
            perror("smash error: getdents failed");
            close(fd);
            return;
        }
        if (nread == 0) {
            break;
        }


        for (int bpos = 0; bpos < nread;) {
            struct linux_dirent64 *d = (struct linux_dirent64 *) (buf + bpos);
            string name = d->d_name;
            bpos += d->d_reclen;

            if (name == "." || name == "..") continue;

            string path = "/sys/bus/usb/devices/" + name;

            // read vendor
            string vendor = "N/A";
            if (read_file_content((path + "/idVendor").c_str(), file_content, 256) > 0){
                    string tmp = _trim(string(file_content));
					if (!tmp.empty())
						vendor = tmp;
			}

            // piazza
           // if (vendor == "1d6b") continue;

            // read id
            //string id_str = "N/A";
            //if (read_file_content((path + "/devnum").c_str(), file_content, 256) > 0)
             //   id_str = _trim(string(file_content));
             usb_device dev;
             dev.id = INT_MAX;
			
			string prod = "N/A";
			dev.manuf = "N/A";
            // read product id
            if (read_file_content((path + "/idProduct").c_str(), file_content, 256) > 0){
                string temp = _trim(string(file_content));
                if(!temp.empty())
					prod = temp;
			}
			
			if (read_file_content((path + "/devnum").c_str(),file_content,256) > 0){
				string tmp = _trim(string(file_content));
				if (!tmp.empty()){
					try {
						dev.id = stoi(tmp);
							} catch (...) {
					dev.id = INT_MAX;
				}
			}	
		}

            
            //dev.id = atoi(id_str.c_str());
            dev.vendor = vendor;
            dev.product = prod;

           if (read_file_content((path + "/manufacturer").c_str(), file_content, 256) > 0){
				string tmp = _trim(string(file_content));		
				if (!tmp.empty())
					dev.manuf = tmp;
			}

            dev.prod_name = "N/A";
            if (read_file_content((path + "/product").c_str(), file_content, 256) > 0){
				string tmp = _trim(string(file_content));
				if (!tmp.empty()) 
					dev.prod_name = tmp;
			}
            dev.power = "N/A";
            if (read_file_content((path + "/bMaxPower").c_str(), file_content, 256) > 0){
                		string tmp = _trim(string(file_content));
				if (!tmp.empty()) 
					dev.power = tmp;
			}
			if (vendor == "N/A" && prod == "N/A"){
				continue;
			}

            list.push_back(dev);
        }
    }
    close(fd);

    if (list.empty()) {
        cerr << "smash error: usbinfo: no USB devices found" << endl;
        return;
    }

    sort(list.begin(), list.end(), [](const usb_device& a, const usb_device& b) {
        return a.id < b.id;
    });

    for (size_t i = 0; i < list.size(); i++) {
			cout << "Device " << list[i].id;
			cout << ": ID "
			<< list[i].vendor << ":" << list[i].product << " "
             << list[i].manuf << " "
             << list[i].prod_name << " MaxPower: "
             << list[i].power << endl;
    }
}
///////////////////////////////////////////////////////////////
    void JobsList::updateMaxJobId() {
        int maxJobId = 0;
        for (size_t i = 0; i < List.size(); ++i) {
            int current = List[i]->getID();
            if (current > maxJobId) {
                maxJobId = current;
            }
        }
        max_id = maxJobId;
    }

    void JobsList::removeJobById(int jobId) {
        for (auto it = List.begin(); it != List.end(); ++it) {
            if ((*it)->getID() == jobId) {
                List.erase(it);
                updateMaxJobId();
                return;
            }
        }
    }

void JobsList::removeJobByPID(int jobPID) {
    for (auto it = List.begin(); it != List.end(); ++it) {
        if((*it)->getPid() == jobPID){
            List.erase(it);
            updateMaxJobId();
            return;
        }
    }
}

    void JobsList::killAllJobs() {
        auto it = List.begin();
        while (it != List.end()) {
            JobEntry *job = *it;
            if (kill(job->getPid(), SIGKILL) == -1) {
                perror("smash error: kill failed");
                return;
            }
            it = List.erase(it);
        }
        max_id = 0;
    }

    JobsList::JobEntry *JobsList::getLastJob() {
        return List.back();
    }


////////////////////////////////////////////////////////////
    JobsList::JobsList():List(), max_id(0){}

    JobsList::JobEntry::JobEntry(Command * c, int job_id, pid_t pid): job_id(job_id),c(c), pid(pid)
    {}
    JobsList::JobEntry::~JobEntry()
    {
        delete c;
    }

    void JobsList::removeFinishedJobs() {
        auto it = List.begin();

        while (it != List.end()) {
            pid_t PID = waitpid((*it)->getPid(), nullptr, WNOHANG);

            if (PID == -1) {
                it = List.erase(it);
            }

            else if (PID > 0) {
                it = List.erase(it);
            } else {
                it++;
            }
        }
        updateMaxJobId();
}

    void JobsList::printJobsList() {
        removeFinishedJobs();
        for (const auto &job: List) {
            if (strcmp(job->getCommand()->getAlias().c_str(), "none") == 0) {
                cout << "[" << job->getID() << "] " << (job->getCommand()->getCmdLine())<< endl;
            } else {
                cout << "[" << job->getID() << "] " << (job->getCommand()->getAliasCmdLine())<< endl;
            }
        }
    }

    void JobsList::addJob(Command *cmd, pid_t pid) {
        JobEntry* NewJob = new JobEntry(cmd, max_id + 1,pid);
        max_id = max_id + 1;
        List.push_back(NewJob);
    }

string ReplaceNickname_Alternative(const vector<pair<string, string>>& pairs,
                                        const string& CmdLine) {
    size_t first_space_index = CmdLine.find_first_of(WHITESPACE);
    string key;
    string args;

    if (first_space_index == string::npos) {
        key = CmdLine;
        args = "";
    }
    else
    {
        key = CmdLine.substr(0, first_space_index);
        args = CmdLine.substr(first_space_index);
    }

    for (const auto& alias_pair : pairs) {
        if (alias_pair.first == key) {
            const string& alias_command = alias_pair.second;
            return alias_command + args;
        }
    }
    return CmdLine;
}

bool isRedirectionNotInsideQuotes(const char* line) {
    if (!line) {
        return false;
    }

    bool insideQuote = false;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '\'') {
            insideQuote = !insideQuote;
        } else if (line[i] == '>') {
            if (!insideQuote) {
                return true;
            }
        }
    }
    return false;
}

bool isAppendRedirectionNotInsideQuotes(const char* line) {
    if (!line) {
        return false;
    }

    bool insideQuote = false;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '\'') {
            insideQuote = !insideQuote;
        } else if (line[i] == '>' && line[i + 1] == '>') {
            if (!insideQuote) {
                return true;
            }
        }
    }
    return false;
}

/////////////////////////////////////////////////////////
    SmallShell::SmallShell():prompt("smash> "), jobs(JobsList()),last_dir("none"),aliases(vector<pair<string, string>>()), current_running_pid(-1),
    smash_pid(getpid()){}

    SmallShell::~SmallShell(){}

    JobsList &SmallShell::getJobs() {
        return jobs;
    }

bool isPipeNotInsideQuotes(const char* cmdStr) {
    if (!cmdStr) {
        return false;
    }

    bool inQuotes = false;
    for (int i = 0; cmdStr[i] != '\0'; i++) {
        if (cmdStr[i] == '\'') {
            inQuotes = !inQuotes;
        }
        else if (cmdStr[i] == '|') {
            if (!inQuotes) {
                return true;
            }
        }
    }
    return false;
}

    Command *SmallShell::CreateCommand(const char *cmd_line) {
        // For example:
        /*
        string cmd_s = _trim(string(cmd_line));
        string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

        if (firstWord.compare("pwd") == 0) {
          return new GetCurrDirCommand(cmd_line);
        }
        else if (firstWord.compare("showpid") == 0) {
          return new ShowPidCommand(cmd_line);
        }
        else if ...
        .....
        else {
          return new ExternalCommand(cmd_line);
        }
        */
        string cmd_s = _trim(string(cmd_line));
        if (cmd_s.empty() == true) {
            return nullptr;
        }
        string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
       // string RealCommand = cmd_s;
       string RealCommand = string(cmd_line); //for the spaces thing in the test
        bool flag = aliasAlreadyExists(aliases, firstWord);
        if (flag) {
            cmd_s = ReplaceNickname_Alternative(aliases, cmd_s);
        }
        firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

        if (cmd_s.find(">>") != string::npos &&
            isAppendRedirectionNotInsideQuotes(cmd_s.c_str()) &&
            !(firstWord == "alias" && (cmd_s.find(">>=") != string::npos))) {
            return new RedirectionCommand(cmd_s.c_str(), ">>");
        }
        if (cmd_s.find('>') != string::npos &&
            isRedirectionNotInsideQuotes(cmd_s.c_str()) &&
            !(firstWord == "alias" && (cmd_s.find(">=") != string::npos))) {

            return new RedirectionCommand(cmd_s.c_str(), ">");
        } else if (cmd_s.find("|&") != string::npos &&
                   isPipeNotInsideQuotes(cmd_s.c_str())) {
            return new PipeCommand(cmd_s.c_str(), 1);
        } else if (cmd_s.find('|') != string::npos &&
                   isPipeNotInsideQuotes(cmd_s.c_str()) &&
                   !(firstWord == "alias" &&
                     (cmd_s.find("|=") != string::npos))) {
            return new PipeCommand(cmd_s.c_str(), 0);
        } else if ((firstWord == "chprompt") || (firstWord == "chprompt&")) {
            return new chpromptCommand(cmd_s.c_str());
        } else if ((firstWord == "showpid") || (firstWord == "showpid&")) {
            return new ShowPidCommand(cmd_s.c_str());
        } else if ((firstWord == "pwd") || (firstWord == "pwd&")) {
            return new GetCurrDirCommand(cmd_s.c_str());
        } else if ((firstWord == "cd") || (firstWord == "cd&")) {
            return new ChangeDirCommand(cmd_s.c_str());
        } else if ((firstWord == "jobs") || (firstWord == "jobs&")) {
            return new jobsCommand(cmd_s.c_str());
        } else if ((firstWord == "fg") || (firstWord == "fg&")) {
            return new ForegroundCommand(cmd_s.c_str());
        } else if ((firstWord == "quit") || (firstWord == "quit&")) {
            return new QuitCommand(cmd_s.c_str());
        } else if ((firstWord == "kill") || (firstWord == "kill&")) {
            return new KillCommand(cmd_s.c_str());
        } else if ((firstWord == "alias") || (firstWord == "alias&")) {
            return new AliasCommand(cmd_s.c_str());
        } else if ((firstWord == "unalias") || (firstWord == "unalias&")) {
            return new UnAliasCommand(cmd_s.c_str());
        } else if ((firstWord == "unsetenv") || (firstWord == "unsetenv&")) {
            return new UnSetEnvCommand(cmd_s.c_str());
        } else if ((firstWord == "sysinfo") || (firstWord == "sysinfo&")) {
            return new SysInfoCommand(cmd_s.c_str());
        } else if ((firstWord == "whoami") || (firstWord == "whoami&")) {
            return new WhoAmICommand(cmd_s.c_str());
        } else if ((firstWord == "du") || (firstWord == "du&")) {
            return new DiskUsageCommand(cmd_s.c_str());
        } else if ((firstWord == "usbinfo") || (firstWord == "usbinfo&")) {
            return new USBInfoCommand(cmd_s.c_str());
        } else {
            getJobs().removeFinishedJobs();
            auto *command = new ExternalCommand(
                    flag ? cmd_s.c_str() : cmd_line);

            if (flag) {
                command->setAliasCmdLine(RealCommand);
                command->setAlias(firstWord);
            }
            return command;
        }
    }


    void SmallShell::executeCommand(const char *cmd_line) {
        // TODO: Add your implementation here
        // for example:
        // Command* cmd = CreateCommand(cmd_line);
        // cmd->execute();
        // Please note that you must fork smash process for some commands (e.g., external commands....)
        getJobs().removeFinishedJobs();
        Command *cmd = CreateCommand(cmd_line);
        if (cmd != nullptr)
        {
            cmd->execute();
            delete cmd;

        }
        else
        {
            return;
        }

    }
