#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <unistd.h> // For fork() and wait()
#include <sys/wait.h> // For wait()
using namespace std;

const int MAX_ACCOUNTS = 1000;
const int MAX_PROCESSES = 100;

// Account structure
struct Account {
    int id;
    string customerId;
    double balance;
    bool active;
    mutex accMutex; // Mutex for account-level synchronization
};

// Process structure
struct Process {
    int tid;         // Transaction ID
    int aid;         // Account ID
    string type;     // Transaction type: Deposit/Withdraw
    double amount;   // Transaction amount
    string status;   // Status: Pending/Completed/Failed
};

// Banking system
class BankSystem {
private:
    Account accounts[MAX_ACCOUNTS];
    Process processes[MAX_PROCESSES];
    int accountCount = 0;
    int processCount = 0;
    int nextAccountId = 1;
    int nextTransactionId = 1;
    mutex bankMutex; // Mutex for bank-level synchronization

public:
    // Create an account
    int createAccount(const string& customerId, double initialBalance) {
        lock_guard<mutex> lock(bankMutex); // Synchronize account creation
        if (accountCount >= MAX_ACCOUNTS) {
            cout << "Error: Maximum account limit reached." << endl;
            return -1;
        }
        if (initialBalance < 0) {
            cout << "Error: Initial balance cannot be negative." << endl;
            return -1;
        }
        int accountId = nextAccountId++;
        accounts[accountCount].id = accountId;
        accounts[accountCount].customerId = customerId;
        accounts[accountCount].balance = initialBalance;
        accounts[accountCount].active = true;
        accountCount++;
        cout << "Account created successfully! Account ID: " << accountId << endl;
        return accountId;
    }

    // Create a transaction process
    int createProcess(int accountId, const string& type, double amount) {
        lock_guard<mutex> lock(bankMutex); // Synchronize process creation
        if (processCount >= MAX_PROCESSES) {
            cout << "Error: Maximum process limit reached." << endl;
            return -1;
        }
        int tid = nextTransactionId++;
        processes[processCount].tid = tid;
        processes[processCount].aid = accountId;
        processes[processCount].type = type;
        processes[processCount].amount = amount;
        processes[processCount].status = "Pending";
        processCount++;
        cout << "Process created successfully! Transaction ID: " << tid << endl;
        return tid;
    }

    // Execute a transaction process
    void executeProcess(int tid) {
        thread([this, tid]() {
            lock_guard<mutex> lock(bankMutex); // Synchronize process execution
            for (int i = 0; i < processCount; i++) {
                if (processes[i].tid == tid) {
                    Process& proc = processes[i];
                    Account* acc = findAccount(proc.aid);
                    if (!acc) {
                        cout << "Error: Account not found for transaction " << tid << endl;
                        proc.status = "Failed";
                        return;
                    }

                    // Synchronize account operations
                    lock_guard<mutex> accLock(acc->accMutex);
                    if (proc.type == "Deposit") {
                        if (proc.amount <= 0) {
                            cout << "Error: Invalid deposit amount." << endl;
                            proc.status = "Failed";
                            return;
                        }
                        acc->balance += proc.amount;
                        proc.status = "Completed";
                        cout << "Transaction " << tid << ": Deposit successful! New balance: " << acc->balance << endl;
                    } else if (proc.type == "Withdraw") {
                        if (proc.amount <= 0 || acc->balance < proc.amount) {
                            cout << "Error: Insufficient funds or invalid withdrawal amount." << endl;
                            proc.status = "Failed";
                            return;
                        }
                        acc->balance -= proc.amount;
                        proc.status = "Completed";
                        cout << "Transaction " << tid << ": Withdrawal successful! New balance: " << acc->balance << endl;
                    } else {
                        cout << "Error: Unknown transaction type." << endl;
                        proc.status = "Failed";
                    }
                    return;
                }
            }
            cout << "Error: Transaction ID not found." << endl;
        }).join(); // Ensure thread completes execution
    }

    // Find an account by ID
    Account* findAccount(int accountId) {
        for (int i = 0; i < accountCount; i++) {
            if (accounts[i].active && accounts[i].id == accountId) {
                return &accounts[i];
            }
        }
        return nullptr;
    }

    // Check account balance
    double checkBalance(int accountId) {
        Account* acc = findAccount(accountId);
        if (!acc) {
            cout << "Error: Invalid account ID." << endl;
            return -1;
        }
        lock_guard<mutex> lock(acc->accMutex); // Synchronize balance check
        return acc->balance;
    }

    // Display all processes
    void printProcesses() {
        lock_guard<mutex> lock(bankMutex);
        cout << "\nProcess Table:" << endl;
        cout << "TID\tAID\tType\t\tAmount\tStatus" << endl;
        for (int i = 0; i < processCount; i++) {
            cout << processes[i].tid << "\t" << processes[i].aid << "\t"
                 << processes[i].type << "\t\t" << processes[i].amount << "\t"
                 << processes[i].status << endl;
        }
    }
};

// Menu for the banking system
void menu(BankSystem& bank) {
    while (true) {
        cout << "\n------ Banking System ------" << endl;
        cout << "1. Create Account" << endl;
        cout << "2. Deposit" << endl;
        cout << "3. Withdraw" << endl;
        cout << "4. Check Balance" << endl;
        cout << "5. Display All Processes" << endl;
        cout << "6. Exit" << endl;
        cout << "Enter your choice: ";
        int choice;
        cin >> choice;

        switch (choice) {
        case 1: {
            string customerId;
            double initialBalance;
            cout << "Enter customer ID: ";
            cin >> customerId;
            cout << "Enter initial balance: ";
            cin >> initialBalance;
            bank.createAccount(customerId, initialBalance);
            break;
        }
        case 2: {
            int accountId;
            double amount;
            cout << "Enter account ID: ";
            cin >> accountId;
            cout << "Enter amount to deposit: ";
            cin >> amount;
            int tid = bank.createProcess(accountId, "Deposit", amount);
            bank.executeProcess(tid);
            break;
        }
        case 3: {
            int accountId;
            double amount;
            cout << "Enter account ID: ";
            cin >> accountId;
            cout << "Enter amount to withdraw: ";
            cin >> amount;
            int tid = bank.createProcess(accountId, "Withdraw", amount);
            bank.executeProcess(tid);
            break;
        }
        case 4: {
            int accountId;
            cout << "Enter account ID: ";
            cin >> accountId;
            double balance = bank.checkBalance(accountId);
            if (balance != -1) {
                cout << "Balance for Account ID " << accountId << ": " << balance << endl;
            }
            break;
        }
        case 5:
            bank.printProcesses();
            break;
        case 6:
            return;
        default:
            cout << "Invalid choice. Please try again." << endl;
        }
    }
}

int main() {
    BankSystem bank;
    menu(bank);
    return 0;
}

