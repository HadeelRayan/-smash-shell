#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>
#include <list>
#include <time.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
class JobsList;
class SmallShell;

class Command {
// TODO: Add your data members
 char* cmd;
 //char* real_cmd;
 bool is_timed_out;
 int stop_time;
 public:
//  Command(const char* cmd_line):cmd(nullptr) {
//      cmd = (char*) malloc(sizeof (char)*strlen(cmd_line));
//      strcpy(cmd,cmd_line);
//      is_timed_out = false;
//      stop_time = -1;
//  }
  Command(const char* cmd_line,bool is_timed_out,int stop_time):cmd(nullptr),is_timed_out(is_timed_out),stop_time(stop_time) {
        cmd = (char*) malloc(sizeof (char)*strlen(cmd_line));
        strcpy(cmd,cmd_line);
  }
  Command(Command& com){
      strcpy(this->cmd,com.cmd);
  }
  virtual ~Command(){}
  virtual void execute() = 0;
  char* getCmd(){
      return cmd;
  }
//  char* gerRealCmd(){
//      return real_cmd;
//  }
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line,bool is_timed_out,int time): Command(cmd_line,is_timed_out,time){}
  //if (cmd_line)
  virtual ~BuiltInCommand() {}
};
class ExternalCommand : public Command {
    std::string firstWord;
    JobsList& jobs;
    char **Args;
    const char* cmd_line;
    int args_num;
 public:
  ExternalCommand(const char* cmd_line, std::string firstWord ,char **Args, int args_num,JobsList& jobs,
                  bool is_timed_out,int time): Command(cmd_line,is_timed_out,time),
                                     firstWord(firstWord), Args(Args),cmd_line(cmd_line),args_num(args_num),jobs(jobs){}
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line,bool is_timed_out,int time): Command(cmd_line,is_timed_out,time){}
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line,bool is_timed_out,int time): Command(cmd_line,is_timed_out,time){}
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
char * plastPwd;
 public:
  ChangeDirCommand(const char* cmd_line, char* plastPwd,bool is_timed_out,int time):
                    BuiltInCommand(cmd_line,is_timed_out,time) ,plastPwd((plastPwd)){}
  virtual ~ChangeDirCommand() {}
  void execute() override;
};


class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmdLine,bool is_timed_out,int time) : BuiltInCommand(cmdLine,is_timed_out,time) {}
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char *cmdLine,bool is_timed_out,int time) : BuiltInCommand(cmdLine,is_timed_out,time) {}
  virtual ~ShowPidCommand() {}
  void execute() override;
};


class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
  QuitCommand(const char* cmd_line, JobsList& jobs,bool is_timed_out,int time) : BuiltInCommand(cmd_line,is_timed_out,time){}
  virtual ~QuitCommand() {}
  void execute() override;
};

class JobEntry {
    Command* cmd;
    int id;
    pid_t pid;
    time_t start_time;
    bool is_stopped;
    bool is_timed_out;
    int stop_time;
    bool is_finished;
public:
    JobEntry(Command* cmd,int id, pid_t pid, bool stopped,bool is_timed_out,int stop_time)
                :cmd(cmd),id(id),pid(pid),start_time(time(nullptr)),is_stopped(stopped),is_timed_out(is_timed_out)
                ,stop_time(stop_time){}
    ~JobEntry() = default;
    int getId(){
        return id;
    }
    pid_t getPid(){
        return pid;
    }
    char* getCmdLine(){
        return cmd->getCmd();
    }

    time_t getTime(){
        return start_time;
    }
    bool isJobStopped(){
        return is_stopped;
    }
    bool isJobFinished(){
        return is_finished;
    }
    bool isTimedOut(){
        return is_timed_out;
    }
    void jobStop(){
        is_stopped = true;
    }
    int getStopTime(){
        return stop_time;
    }
    void jobContinue(){
        is_stopped = false;
    }
    void setPid(pid_t pid_t){
        pid = pid_t;
    }
};

class JobsList {
 public:
    int max_id;
    std::list<JobEntry> jobs_list;
    JobsList():max_id(0),jobs_list(std::list<JobEntry>()){}
  ~JobsList() = default;
  void addJob(Command* cmd, pid_t pid, bool isStopped,bool isTimedOut,int stopTime);
  void printJobsList();
  void printJobsCommands();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
    JobEntry * getJobByPid(int jobPid);
  void removeJobById(int jobId);
    void removeJobByPid(int jobPid);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob();
  // TODO: Add extra methods or modify exisitng ones as needed
};

class ChpromptCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ChpromptCommand(const char *cmdLine,bool is_timed_out,int time) : BuiltInCommand(cmdLine,is_timed_out,time) {}
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList& jobs,bool is_timed_out,int time) : BuiltInCommand(cmd_line,is_timed_out,time){}
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList& jobs,bool is_timed_out,int time) : BuiltInCommand(cmd_line,is_timed_out,time){}
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList& jobs,bool is_timed_out,int time) : BuiltInCommand(cmd_line,is_timed_out,time){}
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList& jobs,bool is_timed_out,int time) : BuiltInCommand(cmd_line,is_timed_out,time){}
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class HeadCommand : public BuiltInCommand {
 public:
  HeadCommand(const char* cmd_line,bool is_timed_out,int time) : BuiltInCommand(cmd_line,is_timed_out,time){}
  virtual ~HeadCommand() {}
  void execute() override;
};

class TimedOutCommand : public BuiltInCommand {
public:
    TimedOutCommand(const char* cmd_line,bool is_timed_out,int time) : BuiltInCommand(cmd_line,is_timed_out,time){}
    virtual ~TimedOutCommand() {}
    void execute() override;
};

class SmallShell {
 public:
    char* Cmd_line;
    char* real_cmd_line;
    char **Args;
    int args_num;
    std::string first_word;
    std::list<char*> pre_directories;
    std::string prompt = "smash";
    JobsList jobs;
    pid_t curr_process_pid;
    //int curr_process_id;
    bool is_stopped;
    bool is_timed_out;
    int stop_time;
    bool fg_com_running;
    SmallShell();
  Command *CreateCommand(const char* cmd_line,bool is_timed_out,int stop_time);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  void stopFgCom(){
      kill(curr_process_pid,SIGSTOP);
      std::cout << "smash: process " << curr_process_pid << " was stopped" << std::endl;
      ExternalCommand* com = new ExternalCommand(Cmd_line,first_word,Args,args_num,jobs,is_timed_out,stop_time);
      if (jobs.getJobByPid(curr_process_pid) == nullptr){
          jobs.addJob(com, curr_process_pid, true,is_timed_out,stop_time);
      }
      else{
          jobs.getJobByPid(curr_process_pid)->jobStop();
      }
      fg_com_running = false;
  }

  void continueProcess(){
      is_stopped = false;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line,bool is_timed_out, int stop_time);
  // TODO: add extra methods as needed
};


#endif //SMASH_COMMAND_H_
