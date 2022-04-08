#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;


void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
	std::cout << "smash: got ctrl-Z" << endl ;
    if(smash.fg_com_running){
        //smash.stopFgCom();
        kill(smash.curr_process_pid,SIGSTOP);
        std::cout << "smash: process " << smash.curr_process_pid << " was stopped" << std::endl;
        ExternalCommand* com = new ExternalCommand(smash.Cmd_line,smash.first_word,smash.Args,smash.args_num,
                                                   smash.jobs,smash.is_timed_out,smash.stop_time);
        if (smash.jobs.getJobByPid(smash.curr_process_pid) == nullptr){
            smash.jobs.addJob(com, smash.curr_process_pid, true,smash.is_timed_out,smash.stop_time);
        }
        else{
            smash.jobs.getJobByPid(smash.curr_process_pid)->jobStop();
        }
        smash.fg_com_running = false;
    }
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-C" << endl ;
   // smash.jobs.removeFinishedJobs();
    if(smash.fg_com_running){
        kill(smash.curr_process_pid,SIGKILL);
        std::cout << "smash: process " << smash.curr_process_pid << " was killed" << endl;
        smash.fg_com_running= false;
        if(smash.jobs.getJobByPid(smash.curr_process_pid) != nullptr){
            smash.jobs.removeJobByPid(smash.curr_process_pid);
        }
    }
}

void alarmHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash: got an alarm" << endl;
    auto jobs = smash.jobs.jobs_list;
    for (auto it = jobs.begin(); it != jobs.end(); it++) {
        if (it->isTimedOut() == false) continue;
        double time_left = difftime(time(nullptr), it->getTime());
        //cout << it->getStopTime() << " " << time_left <<endl;
        if (time_left >= it->getStopTime()) {//we got an alarm
            kill(it->getPid(),SIGKILL);
            int job_id = it->getId();
            smash.jobs.removeJobById(job_id);
            smash.jobs.removeFinishedJobs();
            cout << "smash: " << it->getCmdLine() << " timed out! "<< endl;
        }
    }
    if(smash.fg_com_running){
        kill(smash.curr_process_pid,SIGKILL);
        //smash.stop_time is not working
        cout << "smash: " << smash.real_cmd_line << " timed out! "<< endl;
        smash.fg_com_running= false;
    }
}
