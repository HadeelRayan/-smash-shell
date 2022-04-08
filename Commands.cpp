#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include "Commands.h"

using namespace std;
//SmallShell &global_shell = SmallShell::getInstance();
#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

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

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : Cmd_line(), Args(), args_num(), first_word(), pre_directories(), prompt("smash"), jobs(),
                           curr_process_pid(), is_stopped(), fg_com_running(){
// TODO: add your implementation

}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
char *fromStrToChar(string str) {
    char *last_dr = new char[str.length() + 1];
    int i;
    for (i = 0; i < str.length(); i++) {
        last_dr[i] = str[i];
    }
    last_dr[i] = '\0';
    return last_dr;
}

Command *SmallShell::CreateCommand(const char *cmd_line, bool is_timed_out, int time) {
    if(string(cmd_line)== "") return nullptr;
    string cmd_s = _trim(string(cmd_line));
  //  std::cout <<"******************Command= "<< cmd_s<<" "<<endl;
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    char* t = fromStrToChar(firstWord);
    _removeBackgroundSign(t);
    firstWord=string(t);
    firstWord= _trim(firstWord);
    char **Args = new char *[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd_line, Args);
    if (args_num == 0) {
        return nullptr;
    }
    SmallShell &smash = SmallShell::getInstance();
    smash.curr_process_pid = getpid();
    smash.Args = Args;
    smash.args_num = args_num;
    smash.Cmd_line = new char[strlen(cmd_line)];
    if(!is_timed_out){
        smash.real_cmd_line = new char[strlen(cmd_line)];
        strcpy(smash.real_cmd_line, cmd_line);
    }
    smash.is_timed_out = is_timed_out;
    smash.stop_time = time;
    strcpy(smash.Cmd_line, cmd_line);
    jobs.removeFinishedJobs();
    if (cmd_s.find('>') != string::npos) {
        return new RedirectionCommand(cmd_line, is_timed_out, time);
    } else if (cmd_s.find('|') != string::npos) {
        return new PipeCommand(cmd_line, is_timed_out, time);
    } else if (firstWord.compare("chprompt") == 0) {
        return new ChpromptCommand(cmd_line,is_timed_out,time);
    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line, is_timed_out, time);
    } else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line, is_timed_out, time);
    } else if (firstWord.compare("cd") == 0) {
        char *last_dr;
        //char *buf = NULL;
        if (args_num > 2) {
            std::cerr << "smash error: cd: too many arguments" << endl;
            return nullptr;
        }
        if (strcmp(Args[1], ("-")) == 0) { // check if command is cd -
            last_dr = pre_directories.back();
            if (last_dr == nullptr) {
                std::cerr << "smash error: cd: OLDPWD not set" << endl;
                return nullptr;
            } else pre_directories.pop_back();
        } else{
            last_dr =Args[1];
        }
        return new ChangeDirCommand(cmd_line, last_dr, is_timed_out, time);
    } else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line,jobs,is_timed_out,time);
    } else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line,jobs,is_timed_out,time);
    } else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line,jobs,is_timed_out,time);
    } else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line,jobs,is_timed_out,time);
    } else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line,jobs,is_timed_out,time);
    } else if (firstWord.compare("head") == 0) {
        if (args_num < 2 || args_num > 3) {
            std::cerr << "smash error: head:not enough arguments" << endl;
            return nullptr;
        }
        return new HeadCommand(cmd_line, is_timed_out, time);
    } else if (firstWord.compare("timeout") == 0) {
        if(smash.args_num < 3 || (args_num >=2 && atoi(smash.Args[1]) <= 0)){
            std::cerr<< "smash error: timeout: invalid arguments"<< endl;
            return nullptr;
        }
        return new TimedOutCommand(cmd_line, true, atoi(Args[2]));
    } else return new ExternalCommand(cmd_line, firstWord, Args, args_num, jobs, is_timed_out, time);

    return nullptr;
}

void ShowPidCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    //if(smash.curr_process_pid != getpid() && smash.curr_process_pid != 0){
        std::cout << "smash pid is " << smash.curr_process_pid << endl;
    //}
    //else
     //   std::cout << "smash pid is " << getpid() << endl;
}

void GetCurrDirCommand::execute() {
    char *buf = getcwd(NULL, 0);
    string st = std::string(buf);
    free(buf);
    std::cout << st << endl;
}

void ChangeDirCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    char* last= getcwd(NULL, 0);
    if(strcmp(smash.Args[1] , "..")==0){
        if (chdir("..") == -1 ){
            perror("smash error: chdir failed");
            return;
        }
    }
    else if(chdir(plastPwd) == -1){
        perror("smash error: chdir failed");
        return;
    }
    smash.pre_directories.push_back(last);
}

void ExternalCommand::execute() {
    char *cmd;
    bool is_backg = false;
    SmallShell &smash = SmallShell::getInstance();
    if (cmd_line[strlen(cmd_line) - 1] == '&') {
        cmd = (char *) malloc(strlen(cmd_line));
        strcpy(cmd, cmd_line);
        cmd[strlen(cmd_line) - 1] = 0;
        is_backg = true;
    } else {
        cmd = (char *) malloc(strlen(cmd_line) + 1);
        strcpy(cmd, cmd_line);
        cmd[strlen(cmd_line)] = 0;
    }
    _trim(cmd);
    char first[]= "/bin/bash";
    char sec[]= "-c";
    char *argv[] = {first, sec, cmd, nullptr};
    pid_t child_pid = fork();
    if (child_pid == 0) {//son
        setpgrp();
        execv("/bin/bash", argv);
    } else if (child_pid > 0) {
        //SmallShell &smash = SmallShell::getInstance();
        smash.fg_com_running = false;
        if (!is_backg) {
            smash.curr_process_pid = child_pid;
            smash.fg_com_running = true;
            waitpid(child_pid, nullptr, WUNTRACED);
            smash.fg_com_running = false;
            smash.continueProcess();
        } else {
            if(smash.is_timed_out){
                cmd_line = smash.real_cmd_line;
            }
            ExternalCommand *com = new ExternalCommand(cmd_line, firstWord, Args, args_num, jobs,
                                                       smash.is_timed_out, smash.stop_time);
            jobs.addJob(com, child_pid, false, smash.is_timed_out, smash.stop_time);
        }
    } else {
        perror("smash error: fork failed");
    }
    smash.is_timed_out = false;
}

void SmallShell::executeCommand(const char *cmd_line, bool is_timed_out, int time) {
    Command *cmd = CreateCommand(cmd_line, is_timed_out, time);
    ////////// chprompt command
    if (cmd == nullptr) return;
    cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped, bool isTimedOut, int stopTime) {
    SmallShell &smash = SmallShell::getInstance();
    removeFinishedJobs();
    max_id++;
    JobEntry new_job = JobEntry(cmd, max_id, pid, isStopped, isTimedOut, stopTime);
    jobs_list.push_back(new_job);
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (auto it = jobs_list.begin(); it != jobs_list.end(); it++) {
        string cmd_line1(it->getCmdLine());
        std::cout << "[" << it->getId() << "] " << cmd_line1 << " : ";
        time_t curr_time = time(nullptr);
        std::cout << it->getPid() << " " << (int) (difftime(curr_time, it->getTime())) << " secs ";
        if (it->isJobStopped())
            std::cout << "(stopped)" << endl;
        else
            std::cout << endl;
    }

}

void JobsList::printJobsCommands() {
    removeFinishedJobs();
}

void JobsList::removeFinishedJobs() {
    pid_t pid_t = waitpid(-1, nullptr, WNOHANG);
    while (pid_t > 0) {
        for (auto it = jobs_list.begin(); it != jobs_list.end(); it++) {
            if (it->getPid() == pid_t) {
                jobs_list.erase(it);
                break;
            }
        }
        pid_t = waitpid(-1, nullptr, WNOHANG);
    }
    if (jobs_list.empty())
        max_id = 0;
    else
        max_id = jobs_list.back().getId();
}

void JobsList::killAllJobs() {
    max_id = 0;
    jobs_list.clear();
}

JobEntry *JobsList::getJobById(int jobId) {
    for (auto it = jobs_list.begin(); it != jobs_list.end(); it++) {
        if (it->getId() == jobId) {
            return &(*it);
        }
    }
    return nullptr;
}
JobEntry *JobsList::getJobByPid(int jobPid) {
    for (auto it = jobs_list.begin(); it != jobs_list.end(); it++) {
        if (it->getPid() == jobPid) {
            return &(*it);
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for (auto it = jobs_list.begin(); it != jobs_list.end(); it++) {
        if (it->getId() == jobId) {
            jobs_list.erase(it);
            return;
        }
    }
}

JobEntry *JobsList::getLastStoppedJob() {
    auto temp=jobs_list.begin();
    for (auto it = jobs_list.begin(); it != jobs_list.end(); it++) {
        if (it->isJobStopped()) {
            temp = it;
        }
    }
    return &(*temp);
}

void JobsList::removeJobByPid(int jobPid) {
    for (auto it = jobs_list.begin(); it != jobs_list.end(); it++) {
        if (it->getPid() == jobPid) {
            jobs_list.erase(it);
            return;
        }
    }
}


void RedirectionCommand::execute() {
    if (string(getCmd())== "\n") return;
    string cmd_s = _trim(string(getCmd()));
    char *Command1 = fromStrToChar(cmd_s.substr(0, cmd_s.find_first_of('>')));
    SmallShell &smash = SmallShell::getInstance();
    smash.curr_process_pid = getpid();
    string fn=cmd_s.substr(cmd_s.find_last_of('>')+1, cmd_s.length());
    fn = _trim(fn);
    char* file_name = fromStrToChar(fn);

    if (cmd_s.find(">>") != string::npos) {
        int num_in_FD = open(file_name, O_CREAT | O_RDWR | O_APPEND , S_IRWXU);
        if (num_in_FD == -1) {
            perror("smash error: open failed");
            return;
        }
        if (fork() == 0) { // son
            close(1);
            dup2(num_in_FD, 1);
            smash.executeCommand(Command1, smash.is_timed_out, smash.stop_time);
            close(num_in_FD);
            exit(0);
        } else {
            wait(NULL);
            close(num_in_FD);

        }
    } else {
        int num_in_FD = open(file_name, O_CREAT | O_RDWR | O_TRUNC , S_IRWXU);
        if (num_in_FD == -1) {
            perror("smash error: open failed");
            return;
        }
        if (fork() == 0) { // son
            close(1);
            dup2(num_in_FD, 1);
           close(num_in_FD);
            smash.executeCommand(Command1, smash.is_timed_out, smash.stop_time);
            //write(my_pipe[1],"Hello", 6);
            exit(0);
        } else { // father
            wait(NULL);
            close(num_in_FD);

        }
    }


}

void PipeCommand::execute() {
    int fd[2];
    if (pipe(fd) < 0) {
        perror("smash error: pipe failed");
        return;
    }
    string cmd_s = _trim(string(getCmd()));
    char *Command1 = fromStrToChar(cmd_s.substr(0, cmd_s.find_first_of("|")));
    SmallShell &smash = SmallShell::getInstance();
    smash.curr_process_pid = getpid();
    int std = 1;
    char *Command2;
    if (cmd_s.find('&') != string::npos) {
        Command2 = fromStrToChar(cmd_s.substr(strlen(Command1) + 3, cmd_s.find_last_of('\n')));
        std = 2;
    } else {
        Command2 = fromStrToChar(cmd_s.substr(strlen(Command1) + 2, cmd_s.find_last_of('\n')));
    }
    if (fork() == 0) { // first command
        setpgrp();
        dup2(fd[1], std);
        smash.executeCommand(Command1, smash.is_timed_out, smash.stop_time);
        close(fd[0]);
        close(fd[1]);
        exit(0);
    }else {
        wait(nullptr);
        int in_dup = dup(0);
        dup2(fd[0], 0);
        smash.executeCommand(Command2, smash.is_timed_out, smash.stop_time);
        close(fd[1]);
        close(fd[0]);
        dup2(in_dup, 0);
        close(in_dup);
    }
}


void KillCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.removeFinishedJobs();
    if (smash.args_num != 3) {
        std::cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int sig_num = (-1) * atoi(smash.Args[1]);
    int id = atoi(smash.Args[2]);
    if(sig_num <= 0|| id == 0){
        std::cerr << "smash error: kill: invalid arguments" << endl;
        return ;
    }

    if (smash.jobs.getJobById(id) == nullptr && id!=0) {
        std::cerr << "smash error: kill: job-id " << id << " does not exist" << endl;
        return;
    }
    pid_t curr_pid = smash.jobs.getJobById(id)->getPid();
    if (kill(curr_pid, sig_num) != 0) {
        perror("smash error: kill failed");
    } else {
        std::cout << "signal number " << sig_num << " was sent to pid " << curr_pid << endl;
    }
    return ;
}

void JobsCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.printJobsList();
}

void QuitCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.args_num == 1) {
        exit(0);
    } else if (strcmp(smash.Args[1], "kill") == 0) {
        smash.jobs.removeFinishedJobs();
        int size = smash.jobs.jobs_list.size();
        std::cout << "smash: sending SIGKILL signal to " << size << " jobs: " << endl;
        for (auto it = smash.jobs.jobs_list.begin(); it != smash.jobs.jobs_list.end(); it++) {
            pid_t pid = it->getPid();
            std::cout << pid << ": " << it->getCmdLine() << endl;
            kill(pid, SIGKILL);
        }
        exit(0);
    }
}

void HeadCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    int N;
    int file;
    if (smash.args_num == 2) {
        N = 10;
        file = open(smash.Args[1], O_RDONLY, S_IRWXU);
    } else {
        N = -1 * atoi(smash.Args[1]);
        if (N < 0) {
            cerr << "smash error: head invalid arguments" << endl;
        }
        file = open(smash.Args[2], O_RDONLY, S_IRWXU);
    }
    if (file == -1) {
        perror("smash error: open failed");
        exit(0);
    }
    int res;
    for (int i = 0; i < N; i++) {
        char *buffer = new char[1];
        while ((res = read(file, buffer, 1)) != 0) {
            if (res < 0) {
                perror("smash error: open failed");
                exit(0);
            }
            if (strcmp(buffer, "\n") == 0) {
                if (write(1, "\n", 1) < 0) {
                    perror("smash error: open failed");
                    exit(0);
                }
                break;
            } else {
                if (write(1, buffer, 1) < 0) {
                    perror("smash error: open failed");
                    exit(0);
                }
            }
        }
    }
}

void TimedOutCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    int time = atoi(smash.Args[1]);
    alarm(time);
    string cmd_s = _trim(string(getCmd()));
    string Command = (cmd_s.substr(cmd_s.find_first_of(" ") + 1, cmd_s.find_last_of(" \n")));
    char *new_command = fromStrToChar(Command.substr(Command.find_first_of(" "), Command.length()));
    _trim(new_command);
    smash.executeCommand(new_command, true, time);
}

void ChpromptCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.args_num == 1) {
        smash.prompt = "smash";
    } else smash.prompt = smash.Args[1];
}

void ForegroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.removeFinishedJobs();
    if (smash.args_num == 1 && smash.jobs.jobs_list.empty()) {
        std::cerr << "smash error: fg: jobs list is empty\n";
        return ;
    } else if (smash.args_num == 1) {
        int pidd = smash.jobs.jobs_list.back().getPid();
        if (smash.jobs.jobs_list.back().isJobStopped()) {
            //smash.curr_process_id = smash.jobs.jobs_list.back().getId();
            if (kill(pidd, SIGCONT) != 0) {
                perror("smash error: kill failed");
            }
        }
        int job_id = smash.jobs.jobs_list.back().getId();
        smash.curr_process_pid = pidd;
        //smash.curr_process_id = job_id;
        smash.fg_com_running = true;
        smash.Cmd_line = smash.jobs.jobs_list.back().getCmdLine();
        auto job = smash.jobs.getJobById(job_id);
        std::cout << job->getCmdLine() << " : " << pidd << endl;
        waitpid(pidd, nullptr, WUNTRACED);
        smash.jobs.removeFinishedJobs();
        return ;
    }
    int job_id = atoi(smash.Args[1]);
    if (smash.args_num > 2 || job_id == 0) {
        std::cerr << "smash error: fg: invalid arguments" << endl;
        return;
    } else {
        auto job = smash.jobs.getJobById(job_id);
        if (job == nullptr) {
            std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
            return ;
        }
        int pid = job->getPid();
        if (job->isJobStopped()) {
            if (kill(pid, SIGCONT) != 0) {
                perror("smash error: kill failed");
            }
        }
        //smash.curr_process_id = job_id;
        smash.curr_process_pid = pid;
        smash.fg_com_running = true;
        smash.Cmd_line = job->getCmdLine();
        std::cout << job->getCmdLine() << " : " << pid << endl;
        waitpid(pid, NULL, WUNTRACED);
        smash.jobs.removeFinishedJobs();
    }
    return ;
}

void BackgroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.args_num == 1) {
        if (smash.jobs.jobs_list.empty() || smash.jobs.getLastStoppedJob() == nullptr) {
            std::cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return ;
        }
        int pidd = smash.jobs.getLastStoppedJob()->getPid();
        std::cout << smash.jobs.getLastStoppedJob()->getCmdLine() << " : " << pidd << endl;
        smash.jobs.getLastStoppedJob()->jobContinue();
        if (kill(pidd, SIGCONT) != 0) {
            perror("smash error: kill failed");
        }
        smash.curr_process_pid = pidd;
        smash.fg_com_running = false;
        return ;
    }
    int job_id = atoi(smash.Args[1]);
    if (smash.args_num > 2 || job_id == 0) {
        std::cerr << "smash error: bg: invalid arguments" << endl;
        return ;
    } else {
        auto job = smash.jobs.getJobById(job_id);
        if (job == nullptr) {
            std::cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
            return ;
        }
        int pid = job->getPid();
        if (job->isJobStopped()) {
            std::cout << job->getCmdLine() << " : " << pid << endl;
            job->jobContinue();
            if (kill(pid, SIGCONT) != 0) {
                perror("smash error: kill failed");
            }
            smash.curr_process_pid = pid;
            smash.fg_com_running = false;
            return ;
        } else {
            std::cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
            return ;
        }
    }
    return;
}
