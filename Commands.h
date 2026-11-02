
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
using namespace std;

#include <vector>
#include <algorithm>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)



bool isComplex(string cmdLine);
bool _isBackgroundComamnd(const char *cmd_line);
void _removeBackgroundSign(char *cmd_line);
string _rtrim(const std::string &s);


class Command {
   
protected:
    int args_num;
    bool is_background;
    char* args[COMMAND_MAX_ARGS+2];
    string cmdline;
    string alias;
    string alias_cmd_line;
    string background_cmd_line;



public:
    Command(const char *cmd_line);

    virtual ~Command();

    virtual void execute() = 0;

    string getAlias() const
    {
        return alias;
    }

    void setAlias(const string& new_alias)
    {
        alias = new_alias;
    }

    string getCmdLine() const
    {
       if(is_background){
            return background_cmd_line;
      }
        return cmdline;
    }
    bool isbackground () const {
        return is_background;
    }

    void setCmdLine(const string& new_CmdLine)
    {
        alias = new_CmdLine;
    }

    string getAliasCmdLine() const
    {
        return alias_cmd_line;
    }

    void setAliasCmdLine(const string& new_alias_cmd_line)
    {
        alias_cmd_line = new_alias_cmd_line;
    }
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {
    }

    void execute() override;
};


class RedirectionCommand : public Command {
    string command_type;
    string File;
    string command;

public:
    explicit RedirectionCommand(const char *cmd_line, const string& type);

    virtual ~RedirectionCommand() {
    }

    void execute() override;
};

class PipeCommand : public Command {
    int pipeType;
    string first_cmd;
    string second_cmd;
public:
    PipeCommand(const char *cmd_line, int pipeType);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class DiskUsageCommand : public Command {
public:
    DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class USBInfoCommand : public Command {
public:
    USBInfoCommand(const char *cmd_line);

    virtual ~USBInfoCommand() {
    }

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    ChangeDirCommand(const char *cmd_line);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

class chpromptCommand : public BuiltInCommand
{
    private:
    string new_prompt;

public:
    chpromptCommand(const char *cmd_line);
    virtual ~chpromptCommand() {
    }
    void execute() override;

};


    class jobsCommand : public BuiltInCommand {
    public:
        jobsCommand(const char *cmd_line);

        virtual ~jobsCommand() {
        }

        void execute() override;
    };


    class JobsList;


    class JobsList {
    public:
        class JobEntry {
        private:
            int job_id;
            Command *c;
            pid_t pid;

        public:
            JobEntry(Command *c, int job_id, pid_t pid);

            ~JobEntry();

            int getID() const {
                return job_id;
            }

            Command *getCommand() const {
                return c;
            }

            pid_t getPid() const {
                return pid;
            }

        };


        vector<JobEntry*> List;
        int max_id;

        JobsList();

        ~JobsList() = default;

        void addJob(Command *cmd, pid_t pid);

        void printJobsList();

        void killAllJobs();

        void removeFinishedJobs();

        JobEntry *getJobById(int jobId);

        void removeJobById(int jobId);

        void removeJobByPID(int jobPID);

        JobEntry *getLastJob(int *lastJobId);

        JobEntry *getLastStoppedJob(int *jobId);


        void updateMaxJobId();

        JobEntry *getLastJob();

        void updatingAliasesInjobList(const string& alias);

    };

class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

    class QuitCommand : public BuiltInCommand {
    public:
        QuitCommand(const char *cmd_line);

        virtual ~QuitCommand() {
        }

        void execute() override;
    };

    class JobsCommand : public BuiltInCommand {
    public:
        JobsCommand(const char *cmd_line, JobsList *jobs);

        virtual ~JobsCommand() {
        }

        void execute() override;
    };

    class KillCommand : public BuiltInCommand {
    public:
        KillCommand(const char *cmd_line);

        virtual ~KillCommand() {
        }

        void execute() override;
    };

    class ForegroundCommand : public BuiltInCommand {
    public:
        ForegroundCommand(const char *cmd_line);

        virtual ~ForegroundCommand() {
        }

        void execute() override;
    };


    class AliasCommand : public BuiltInCommand {
    public:
        AliasCommand(const char *cmd_line);

        virtual ~AliasCommand() {
        }

        void execute() override;
    };


    class UnSetEnvCommand : public BuiltInCommand {
    public:
        UnSetEnvCommand(const char *cmd_line);

        virtual ~UnSetEnvCommand() {
        }

        void execute() override;
    };

    class SysInfoCommand : public BuiltInCommand {
    public:
        SysInfoCommand(const char *cmd_line);

        virtual ~SysInfoCommand() {
        }

        void execute() override;
    };

    class SmallShell {
    private:
        string prompt;
        JobsList jobs;
        string last_dir;
        vector<pair<string, string>> aliases;

        SmallShell();

    public:
        pid_t current_running_pid;
        pid_t smash_pid;

        Command *CreateCommand(const char *cmd_line);

        SmallShell(SmallShell const &) = delete; // disable copy ctor
        void operator=(SmallShell const &) = delete; // disable = operator
        static SmallShell &getInstance() // make SmallShell singleton
        {
            static SmallShell instance; // Guaranteed to be destroyed.
            // Instantiated on first use.
            return instance;
        }

        string getLastDir() const {
            return last_dir;
        }
        string getPrompt() const {
            return prompt;
        }
        vector<pair<string, string>> &get_aliases() {
            return aliases;
        }

        void setLastDir(const string &newDir) {
            last_dir = newDir;
        }

        void set_prompt(const string &new_p) {
            prompt = new_p + "> ";
        }

        JobsList &getJobs();

        ~SmallShell();

        void executeCommand(const char *cmd_line);

    };

#endif //SMASH_COMMAND_H_
