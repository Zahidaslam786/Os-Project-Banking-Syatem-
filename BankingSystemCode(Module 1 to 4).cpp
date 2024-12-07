// --------------------- VARIBALE SET MODULE1 TO 4 COMPELETE CODE ----------------

#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <iomanip>
using namespace std;

const int MAX_ACCS = 1000;
const int MAX_PROCS = 100;

// Account structure
struct Acc {
    int accId;
    string custId;
    double balance;
    bool active;
    mutex accMutex;

    // Constructor for initialization
    Acc(int accId, const string &custId, double balance, bool active)
        : accId(accId), custId(custId), balance(balance), active(active) {}
};

// Process structure
struct Proc {
    int tid;
    int accId;
    string type;
    double amount;
    string status;
    int burstTime;     // CPU Burst Time
    int remTime;       // Time left for completion
    int waitTime;      // Waiting time for each process
    int turnTime;      // Turnaround time for each process
    int startTime;     // Time when the process started
    int endTime;       // Time when the process ended
};

// Banking system
class BankSystem {
private:
    Acc* accs[MAX_ACCS];
    Proc procs[MAX_PROCS];
    int accCnt = 0;
    int procCnt = 0;
    int nextAccId = 1;
    int nextTid = 1;
    mutex bankMutex;

    Acc* getAccById(int accId) {
        for (int i = 0; i < accCnt; i++) {
            if (accs[i]->accId == accId && accs[i]->active) {
                return accs[i];
            }
        }
        return nullptr;
    }

public:
    // Create an account
    int createAcc(const string &custId, double initBalance) {
        lock_guard<mutex> lock(bankMutex);
        if (accCnt >= MAX_ACCS) {
            cout << "Error: Maximum account limit reached." << endl;
            return -1;
        }
        if (initBalance < 0) {
            cout << "Error: Initial balance cannot be negative." << endl;
            return -1;
        }
        int accId = nextAccId++;
        accs[accCnt] = new Acc(accId, custId, initBalance, true);
        accCnt++;
        cout << "Account created successfully! Account ID: " << accId << endl;
        return accId;
    }

    // Create a transaction process
    int createProc(int accId, const string &type, double amount, int burstTime = 5) {
        lock_guard<mutex> lock(bankMutex);
        Acc* acc = getAccById(accId);
        if (!acc) {
            cout << "Error: Account not found or inactive." << endl;
            return -1;
        }
        if (type == "Withdraw" && acc->balance < amount) {
            cout << "Error: Insufficient funds for withdrawal." << endl;
            return -1;
        }
        if (procCnt >= MAX_PROCS) {
            cout << "Error: Maximum process limit reached." << endl;
            return -1;
        }
        int tid = nextTid++;
        procs[procCnt] = {tid, accId, type, amount, "Pending", burstTime, burstTime, 0, 0, -1, -1};
        procCnt++;
        cout << "Process created successfully! Transaction ID: " << tid << endl;
        return tid;
    }

    // Check account balance
    void checkAccBalance(int accId) {
        lock_guard<mutex> lock(bankMutex);
        Acc* acc = getAccById(accId);
        if (!acc) {
            cout << "Error: Account not found or inactive." << endl;
            return;
        }
        cout << "Account ID: " << accId << "\nCustomer ID: " << acc->custId
             << "\nBalance: $" << fixed << setprecision(2) << acc->balance << endl;
    }

    // Execute transactions in separate threads
    void execProcs() {
        thread procThreads[MAX_PROCS];
        for (int i = 0; i < procCnt; i++) {
            procThreads[i] = thread(&BankSystem::processProc, this, ref(procs[i]));
        }

        for (int i = 0; i < procCnt; i++) {
            procThreads[i].join();
        }
    }

    // Process a single transaction
    void processProc(Proc& proc) {
        Acc* acc = getAccById(proc.accId);
        if (!acc) {
            cout << "Error: Account not found for transaction ID " << proc.tid << endl;
            return;
        }

        lock_guard<mutex> lock(acc->accMutex);
        if (proc.type == "Deposit") {
            acc->balance += proc.amount;
        } else if (proc.type == "Withdraw") {
            acc->balance -= proc.amount;
        }
        proc.status = "Completed";
        cout << "Transaction TID " << proc.tid << " (" << proc.type << ") processed successfully.\n";
    }

    // Print all processes
    void printProcs() {
        lock_guard<mutex> lock(bankMutex);
        cout << "\nProcess Table:\n";
        cout << "TID\tAID\tType\tAmount\tStatus\n";
        for (int i = 0; i < procCnt; i++) {
            cout << procs[i].tid << "\t" << procs[i].accId << "\t"
                 << procs[i].type << "\t" << procs[i].amount << "\t"
                 << procs[i].status << endl;
        }
    }

    // Round Robin Scheduling
    void roundRobinSched(int timeQuantum) {
        cout << "\nExecuting Round Robin Scheduling with Time Quantum: " << timeQuantum << " units\n";
        int currTime = 0;
        bool procsLeft = true;
        int totalCpuTime = 0; // Track total CPU time for CPU utilization
        int totalWaitTime = 0; // Track total waiting time for average calculation
        int totalTurnTime = 0; // Track total turnaround time for average calculation

        // Print the Gantt Chart
        cout << "\nGantt Chart:\n|";
        while (procsLeft) {
            procsLeft = false;
            for (int i = 0; i < procCnt; i++) {
                if (procs[i].remTime > 0) {
                    procsLeft = true;
                    cout << " T" << procs[i].tid << " |"; // Mark the process in the Gantt chart
                    int timeSlice = min(timeQuantum, procs[i].remTime);
                    procs[i].remTime -= timeSlice;
                    currTime += timeSlice;
                    totalCpuTime += timeSlice; // Add time slice to CPU usage

                    // Update waiting time and turnaround time for remaining processes
                    for (int j = 0; j < procCnt; j++) {
                        if (j != i && procs[j].remTime > 0) {
                            procs[j].waitTime += timeSlice;
                        }
                    }

                    if (procs[i].remTime == 0) {
                        procs[i].turnTime = currTime; // Turnaround time is the time at which the process completes
                        procs[i].status = "Completed"; // Mark the process as completed
                        processProc(procs[i]);
                    }
                }
            }
        }

        cout << endl;
        // Print the scheduling metrics after the processes are completed
        printSchedMetrics(currTime, totalCpuTime);
    }

    void printSchedMetrics(int totalTime, int totalCpuTime) {
        double avgWaitTime = 0, avgTurnTime = 0;
        double cpuUtilization = (double)totalCpuTime / totalTime * 100; // Calculate CPU utilization

        cout << "\nProcess Metrics:\n";
        cout << "TID\tBurst\tWaitTime\tTurnTime\tStatus\n";
        for (int i = 0; i < procCnt; i++) {
            avgWaitTime += procs[i].waitTime;
            avgTurnTime += procs[i].turnTime;
            cout << procs[i].tid << "\t" << procs[i].burstTime << "\t"
                 << procs[i].waitTime << "\t" << procs[i].turnTime
                 << "\t\t" << procs[i].status << endl;
        }

        avgWaitTime /= procCnt;
        avgTurnTime /= procCnt;
        cout << fixed << setprecision(2);
        cout << "\nAverage Waiting Time: " << avgWaitTime << " units\n";
        cout << "Average Turnaround Time: " << avgTurnTime << " units\n";
        cout << "CPU Utilization: " << cpuUtilization << "%" << endl; // Display CPU utilization
    }
};

// Menu
void menu(BankSystem &bank) 
{
    while (true) {
        cout << "\n------ Banking System ------\n";
        cout << "1. Create Account\n";
        cout << "2. Deposit\n";
        cout << "3. Withdraw\n";
        cout << "4. Check Balance\n";
        cout << "5. Display All Processes\n";
        cout << "6. Execute Round Robin Scheduling\n";
        cout << "7. Execute All Transactions (Multithreading)\n";
        cout << "8. Exit\n";
        cout << "Enter option: ";
        int option;
        cin >> option;

        switch (option) {
        case 1: {
            string custId;
            double initBalance;
            cout << "Enter Customer ID: ";
            cin >> custId;
            cout << "Enter Initial Balance: ";
            cin >> initBalance;
            bank.createAcc(custId, initBalance);
            break;
        }
        case 2: {
            int accId;
            double amount;
            cout << "Enter Account ID: ";
            cin >> accId;
            cout << "Enter Deposit Amount: ";
            cin >> amount;
            bank.createProc(accId, "Deposit", amount);
            break;
        }
        case 3: {
            int accId;
            double amount;
            cout << "Enter Account ID: ";
            cin >> accId;
            cout << "Enter Withdrawal Amount: ";
            cin >> amount;
            bank.createProc(accId, "Withdraw", amount);
            break;
        }
        case 4: {
            int accId;
            cout << "Enter Account ID: ";
            cin >> accId;
            bank.checkAccBalance(accId);
            break;
        }
        case 5:
            bank.printProcs();
            break;
        case 6: {
            int timeQuantum;
            cout << "Enter Time Quantum for Round Robin: ";
            cin >> timeQuantum;
            bank.roundRobinSched(timeQuantum);
            break;
        }
        case 7:
            bank.execProcs();
            break;
        case 8:
            cout << "Exiting...\n";
            return;
        default:
            cout << "Invalid option.\n";
        }
    }
}

int main() 
{
    BankSystem bank;
    menu(bank);
    return 0;
}


