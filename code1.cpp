

#include <iostream>
#include <string>
#include <mutex>
using namespace std;
const int MaxAcc = 1000;
const int MaxProc = 100;
struct Acc {
    int id;
    string custId;
    double bal;
    bool act;
};
struct Proc {
    int tid; // Trans ID
    int aid; // Acc ID
    string type; // Deposit or Withdraw
    double amt;
    string stat; // Pending, Completed, Failed
};
class BankSys {
public:
    Acc accs[MaxAcc];
    Proc procs[MaxProc];
    int nextAid = 1; // Next Acc ID
    int nextTid = 1; // Next Trans ID
    int accCnt = 0;  // Account count
    int procCnt = 0; // Process count
    mutex mtx;
    // Find an account by ID
    Acc* find_acc(int id) {
        for (int i = 0; i < accCnt; i++) {
            if (accs[i].act && accs[i].id == id) {
                return &accs[i];
            }
        }
        return nullptr;
    }
    // Create an account
    int create_acc(const string& cid, double bal) {
        lock_guard<mutex> lock(mtx);
        if (accCnt >= MaxAcc) {
            cout << "Error: Max accounts reached "<<endl;
            return -1;
        }
        if (bal < 0) {
            cout << "Error: Negative balance not allowed "<<endl;
            return -1;
        }
        int id = nextAid++;
        accs[accCnt++] = { id, cid, bal, true };
        return id;
    }
    // Create a process
    int create_proc(int aid, const string& type, double amt) {
        lock_guard<mutex> lock(mtx);
        if (procCnt >= MaxProc) {
            cout << "Error: Max processes reached "<<endl;
            return -1;
        }
        int tid = nextTid++;
        procs[procCnt++] = { tid, aid, type, amt, "Pending" };
        return tid;
    }
    // Execute a process
    bool exec_proc(int tid) {
        lock_guard<mutex> lock(mtx);
        for (int i = 0; i < procCnt; i++) {
            if (procs[i].tid == tid) {
                Proc& p = procs[i];
                Acc* a = find_acc(p.aid);
                if (!a) {
                    cout << "Error: Account not found for transaction " << tid << endl;
                    p.stat = "Failed";
                    return false;
                }
                if (p.type == "Deposit") {
                    if (p.amt <= 0) {
                        cout << "Error: Invalid deposit amount "<<endl;
                        p.stat = "Failed";
                        return false;
                    }
                    a->bal =a->bal+ p.amt;
                    p.stat = "Completed";
                }
                else if (p.type == "Withdraw") {
                    if (p.amt <= 0 || a->bal < p.amt) {
                        cout << "Error: Insufficient funds or invalid withdrawal amount.\n";
                        p.stat = "Failed";
                        return false;
                    }
                    a->bal =a->bal- p.amt;
                    p.stat = "Completed";
                }
                else {
                    cout << "Error: Unknown transaction type "<<endl;
                    p.stat = "Failed";
                    return false;
                }
                return true;
            }
        }
        cout << "Error: Transaction not found.\n";
        return false;
    }                                 // Check account balance
    double check_bal(int id) {
        lock_guard<mutex> lock(mtx);
        Acc* a = find_acc(id);
        if (!a) {
            cout << "Error: Account not found "<<endl;
            return -1;
        }
        return a->bal;
    }                      // Display all processes
    void print_procs() {
        lock_guard<mutex> lock(mtx);
        cout << endl;
        cout << "Process Table : " << endl;
        cout << "TID\tAID\tType\t\tAmount\tStatus"<<endl;
        for (int i = 0; i < procCnt; i++) {
            cout << procs[i].tid << "\t" << procs[i].aid << "\t"
                << procs[i].type << "\t\t" << procs[i].amt << "\t"
                << procs[i].stat << endl;
        }
    }
};
int main() {
    BankSys bank;

    int bal1;
    cout << "Enter money for creating your account: ";
    cin >> bal1;
    int aid = bank.create_acc("cust1", bal1);
    if (aid != -1) {
        cout << "Account " << aid << " created with balance: " << bank.check_bal(aid) << endl;
    }

    int deposit;
    cout << "Enter the amount to deposit: ";
    cin >> deposit;
    int depTid = bank.create_proc(aid, "Deposit", deposit);
    bank.print_procs(); // Display processes before execution
    if (bank.exec_proc(depTid)) {
        cout << "After deposit, balance: " << bank.check_bal(aid) << endl;
    }
    bank.print_procs(); // Display processes after execution

    int withdraw;
    cout << "Enter the amount to withdraw: ";
    cin >> withdraw;
    int wTid = bank.create_proc(aid, "Withdraw", withdraw);
    bank.print_procs(); // Display processes before execution
    if (bank.exec_proc(wTid)) {
        cout << "After withdrawal, balance: " << bank.check_bal(aid) << endl;
    }
    bank.print_procs(); // Display processes after execution
    return 0;
}