// ===========================================================
// Client & Transactions Management System
// ===========================================================
// A console-based CRUD application for managing bank clients,
// deposit/withdraw transactions, and user accounts with
// permission-based access control (login required).
//
// Data is persisted in flat text files, one record per line,
// with fields separated by the "#//#" token:
//   Clients.txt: AccountNumber#//#PinCode#//#Name#//#Phone#//#AccountBalance
//   Users.txt:   UserName#//#Password#//#Permissions
//
// Permissions are a bitmask (see enPermissions) so a user can be
// granted any combination of menu actions; -1 means full access.
// ===========================================================

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <limits>
#include <utility>

using namespace std;
const string ClientsFileName = "Clients.txt";
const string UsersFileName = "Users.txt";

struct sUser
{
    string UserName;
    string Password;

    // -1 means unrestricted access (see enPermissions); any other
    // value is a bitmask of the individual permissions this user holds.
    short  Permissions = -1;

    // Soft-delete flag, same convention as sClient::MarkForDelete below.
    bool MarkForDelete = false;
};

struct sClient
{
    string AccountNumber;
    string PinCode;
    string Name;
    string Phone;
    double AccountBalance;

    // Soft-delete flag: when true, the record is skipped when
    // saving to file (see SaveClientsDataToFile), rather than
    // being removed from the in-memory vector immediately.
    bool MarkForDelete = false;


};

// Named permission flags, combined via bitwise OR (e.g. pListClients | pAddNewClient).
// pAll (-1) has every bit set and represents unrestricted access.
enum enPermissions
{
    pAll = -1,
    pListClients = 1 << 0, // 1
    pAddNewClient = 1 << 1, // 2
    pDeleteClient = 1 << 2, // 4
    pUpdateClient = 1 << 3, // 8
    pFindClient = 1 << 4, // 16
    pTransactions = 1 << 5, // 32
    pManageUsers = 1 << 6  // 64
};

void ShowMainMenu(sUser User);
void ShowTransactionsMenu(sUser User);
void ShowLoginScreen();
void ShowManageUsersMenu(sUser User);
void ShowAccessDeniedScreen();
bool HasPermission(short Permissions, enPermissions Permission);



// Splits a string into tokens using the given delimiter.
// Empty tokens (e.g. from consecutive delimiters) are skipped.
vector<string> SplitString(string S1, string Delim)
{

    vector<string> vString;

    short pos = 0;
    string sWord; // define a string variable

    // use find() function to get the position of the delimiters
    while ((pos = S1.find(Delim)) != std::string::npos)
    {
        sWord = S1.substr(0, pos); // store the word
        if (sWord != "")
        {
            vString.push_back(sWord);
        }

        S1.erase(0, pos + Delim.length());  /* erase() until position and move to next word. */
    }

    if (S1 != "")
    {
        vString.push_back(S1); // it adds last word of the string.
    }

    return vString;

}

// Parses a single line from the data file into a client record.
sClient ConvertClientLineToRecord(string Line, string Separator = "#//#")
{

    sClient Client;
    vector<string> vClientData;

    vClientData = SplitString(Line, Separator);

    Client.AccountNumber = vClientData[0];
    Client.PinCode = vClientData[1];
    Client.Name = vClientData[2];
    Client.Phone = vClientData[3];
    Client.AccountBalance = stod(vClientData[4]);//cast string to double


    return Client;

}

// Parses a single line from Users.txt into a user record. Mirrors
// ConvertClientLineToRecord, same "#//#"-delimited format.
sUser ConvertUserLineToRecord(string Line, string Separator = "#//#")
{

    sUser User;
    vector<string> vUserData;

    vUserData = SplitString(Line, Separator);

    User.UserName = vUserData[0];
    User.Password = vUserData[1];
    User.Permissions = stoi(vUserData[2]);


    return User;
}

// Serializes a client record into a single delimited line for storage.
string ConvertClientRecordToLine(sClient Client, string Separator = "#//#")
{

    string stClientRecord = "";

    stClientRecord += Client.AccountNumber + Separator;
    stClientRecord += Client.PinCode + Separator;
    stClientRecord += Client.Name + Separator;
    stClientRecord += Client.Phone + Separator;
    stClientRecord += to_string(Client.AccountBalance);

    return stClientRecord;

}

// Serializes a user record into a single delimited line for storage.
// Mirrors ConvertClientRecordToLine.
string ConvertUserRecordToLine(sUser User, string Separator = "#//#")
{

    string stUserRecord = "";

    stUserRecord += User.UserName + Separator;
    stUserRecord += User.Password + Separator;
    stUserRecord += to_string(User.Permissions);


    return stUserRecord;

}
// Checks whether a client with the given account number already
// exists in the file, reading directly from disk (used only
// during new-client creation, before a vector is loaded).
bool ClientExistsByAccountNumber(string AccountNumber, string FileName)
{

    vector <sClient> vClients;

    fstream MyFile;
    MyFile.open(FileName, ios::in);//read Mode

    if (MyFile.is_open())
    {

        string Line;
        sClient Client;

        while (getline(MyFile, Line))
        {

            Client = ConvertClientLineToRecord(Line);
            if (Client.AccountNumber == AccountNumber)
            {
                MyFile.close();
                return true;
            }


            vClients.push_back(Client);
        }

        MyFile.close();

    }

    return false;


}

// Reads a double from input, re-prompting if the entered value
// is not a valid number (clears cin's fail state and discards
// the bad input so the program doesn't get stuck in a loop).
double ReadValidatedDouble(string Prompt)
{
    double Value = 0;
    cout << Prompt;
    cin >> Value;

    while (cin.fail())
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input, please enter a numeric value? ";
        cin >> Value;
    }

    return Value;
}

// Same idea as ReadValidatedDouble, for whole-number menu choices.
short ReadValidatedShort(string Prompt)
{
    short Value = 0;
    cout << Prompt;
    cin >> Value;

    while (cin.fail())
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input, please enter a number? ";
        cin >> Value;
    }

    return Value;
}

// Prompts the user for all fields of a new client, re-prompting
// for the account number until a unique one is entered.
sClient ReadNewClient()
{
    sClient Client;

    cout << "Enter Account Number? ";

    // Usage of std::ws will extract all the whitespace character
    getline(cin >> ws, Client.AccountNumber);

    while (ClientExistsByAccountNumber(Client.AccountNumber, ClientsFileName))
    {
        cout << "\nClient with [" << Client.AccountNumber << "] already exists, Enter another Account Number? ";
        getline(cin >> ws, Client.AccountNumber);
    }


    cout << "Enter PinCode? ";
    getline(cin, Client.PinCode);

    cout << "Enter Name? ";
    getline(cin, Client.Name);

    cout << "Enter Phone? ";
    getline(cin, Client.Phone);

    Client.AccountBalance = ReadValidatedDouble("Enter AccountBalance? ");

    return Client;

}

string ReadUserName()
{
    string UserName = "";
    cout << "Please enter UserName? ";
    cin >> UserName;
    return UserName;
}

bool ReadUserChoice(string message)
{
    char choice;
    cout << "\n" + message + " ? y/n ? ";
    cin >> choice;

    return (choice == 'y' || choice == 'Y' || choice == '1');
}

string ReadUserPassword()
{
    string Password = "";
    cout << "Enter Password? ";
    cin >> Password;
    return Password;
}

sUser ReadNewUser()
{
    sUser User;
    User.UserName = ReadUserName();
    User.Password = ReadUserPassword();
    return User;
}
// Reads all client records from disk into memory.
// Returns an empty vector if the file doesn't exist or can't be opened.

vector <sClient> LoadClientsDataFromFile(string FileName)
{

    vector <sClient> vClients;

    fstream MyFile;
    MyFile.open(FileName, ios::in);//read Mode

    if (MyFile.is_open())
    {

        string Line;
        sClient Client;

        while (getline(MyFile, Line))
        {

            Client = ConvertClientLineToRecord(Line);

            vClients.push_back(Client);
        }

        MyFile.close();

    }

    return vClients;

}

// Reads all user records from disk into memory. Mirrors
// LoadClientsDataFromFile.
vector <sUser> LoadUsersDataFromFile(string FileName)
{

    vector <sUser> vUsers;

    fstream MyFile;
    MyFile.open(FileName, ios::in);//read Mode

    if (MyFile.is_open())
    {

        string Line;
        sUser User;

        while (getline(MyFile, Line))
        {

            User = ConvertUserLineToRecord(Line);

            vUsers.push_back(User);
        }

        MyFile.close();

    }

    return vUsers;

}

void PrintClientRecordLine(sClient Client)
{

    cout << "| " << setw(15) << left << Client.AccountNumber;
    cout << "| " << setw(10) << left << Client.PinCode;
    cout << "| " << setw(40) << left << Client.Name;
    cout << "| " << setw(12) << left << Client.Phone;
    cout << "| " << setw(12) << left << Client.AccountBalance;

}

void PrintUserRecordLine(sUser User)
{
    cout << "| " << setw(15) << left << User.UserName;
    cout << "| " << setw(10) << left << User.Password;
    cout << "| " << setw(40) << left << User.Permissions;
}

void PrintClientRecordBalanceLine(sClient Client)
{

    cout << "| " << setw(15) << left << Client.AccountNumber;
    cout << "| " << setw(40) << left << Client.Name;
    cout << "| " << setw(12) << left << Client.AccountBalance;

}

// Permission check lives here, inside the screen it protects, rather
// than only at the menu dispatch site (see enMainMenuOptions switch
// below). This way the guard travels with the function even if
// something else ever calls it directly.
void ShowAllClientsScreen(sUser User)
{
    if (!HasPermission(User.Permissions, pListClients))
    {
        ShowAccessDeniedScreen();
        return;
    }

    vector <sClient> vClients = LoadClientsDataFromFile(ClientsFileName);

    cout << "\n\t\t\t\t\tClient List (" << vClients.size() << ") Client(s).";
    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;

    cout << "| " << left << setw(15) << "Account Number";
    cout << "| " << left << setw(10) << "Pin Code";
    cout << "| " << left << setw(40) << "Client Name";
    cout << "| " << left << setw(12) << "Phone";
    cout << "| " << left << setw(12) << "Balance";
    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;

    if (vClients.size() == 0)
        cout << "\t\t\t\tNo Clients Available In the System!";
    else

        for (sClient Client : vClients)
        {

            PrintClientRecordLine(Client);
            cout << endl;
        }

    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;

}

void ShowTotalBalances()
{

    vector <sClient> vClients = LoadClientsDataFromFile(ClientsFileName);

    cout << "\n\t\t\t\t\tBalances List (" << vClients.size() << ") Client(s).";
    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;

    cout << "| " << left << setw(15) << "Account Number";
    cout << "| " << left << setw(40) << "Client Name";
    cout << "| " << left << setw(12) << "Balance";
    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;

    double TotalBalances = 0;

    if (vClients.size() == 0)
        cout << "\t\t\t\tNo Clients Available In the System!";
    else

        for (sClient Client : vClients)
        {

            PrintClientRecordBalanceLine(Client);
            TotalBalances += Client.AccountBalance;

            cout << endl;
        }

    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;
    cout << "\t\t\t\t\t   Total Balances = " << TotalBalances;

}

// Lists all users. Unlike the client/user CRUD screens below, this
// one has no permission check of its own — access is gated once, at
// the ShowManageUsersMenu entry point, since every action inside
// that menu is available to anyone who can open it.
void ShowAllUsersScreen()
{
    vector <sUser> vUsers = LoadUsersDataFromFile(UsersFileName);
    cout << "\n\t\t\t\t\tUser List (" << vUsers.size() << ") User(s).";
    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;
    cout << "| " << left << setw(15) << "User Name";
    cout << "| " << left << setw(10) << "Password";
    cout << "| " << left << setw(40) << "Permissions";
    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;
    if (vUsers.size() == 0)
        cout << "\t\t\t\tNo Users Available In the System!";
    else
        for (sUser User : vUsers)
        {
            PrintUserRecordLine(User);
            cout << endl;
        }
    cout << "\n_______________________________________________________";
    cout << "_________________________________________\n" << endl;
}

void ShowAccessDeniedScreen()
{
    cout << "\n-----------------------------------\n";
    cout << "Access Denied,";
    cout << "\nYou don't Have Permission To Do this,";
    cout << "\nPlease Contact Your Admin";
    cout << "\n-----------------------------------\n";
}

void PrintClientCard(sClient Client)
{
    cout << "\nThe following are the client details:\n";
    cout << "-----------------------------------";
    cout << "\nAccount Number: " << Client.AccountNumber;
    cout << "\nPin Code     : " << Client.PinCode;
    cout << "\nName         : " << Client.Name;
    cout << "\nPhone        : " << Client.Phone;
    cout << "\nAccount Balance: " << Client.AccountBalance;
    cout << "\n-----------------------------------\n";

}

void PrintUserCard(sUser User)
{
    cout << "\nThe following are the user details:\n";
    cout << "-----------------------------------";
    cout << "\nUser Name  : " << User.UserName;
    cout << "\nPassword   : " << User.Password;
    cout << "\nPermissions: " << User.Permissions;
    cout << "\n-----------------------------------\n";
}

// Searches an already-loaded vector for a client with the given
// account number. Used by all screens that operate on a loaded
// client list (Delete, Update, Find, Deposit, Withdraw).
bool FindClientByAccountNumber(string AccountNumber, vector <sClient> vClients, sClient& Client)
{

    for (sClient C : vClients)
    {

        if (C.AccountNumber == AccountNumber)
        {
            Client = C;
            return true;
        }

    }
    return false;

}

// Checks a login attempt against the loaded user list. On success,
// fills in the matched user's Permissions so the caller knows what
// they're allowed to do for the rest of the session.
bool FindUserByUserNameAndPassword(vector <sUser> vUsers, sUser& User)
{

    for (sUser U : vUsers)
    {

        if (U.UserName == User.UserName && U.Password == User.Password)
        {
            User.Permissions = U.Permissions;
            return true;
        }

    }
    return false;

}

// Looks up a user by name only (no password check) — used by the
// User-management screens (Delete/Update/Find/Add), which already
// require the caller to hold Manage Users permission to reach them.
bool FindUserByUserName(vector <sUser> vUsers, sUser& User)
{
    for (sUser U : vUsers)
    {
        if (U.UserName == User.UserName)
        {
            User.Permissions = U.Permissions;
            User.Password = U.Password;
            return true;
        }
    }
    return false;
}

sClient ChangeClientRecord(string AccountNumber)
{
    sClient Client;

    Client.AccountNumber = AccountNumber;

    cout << "\n\nEnter PinCode? ";
    getline(cin >> ws, Client.PinCode);

    cout << "Enter Name? ";
    getline(cin, Client.Name);

    cout << "Enter Phone? ";
    getline(cin, Client.Phone);

    Client.AccountBalance = ReadValidatedDouble("Enter AccountBalance? ");

    return Client;

}

// Each permission name is paired directly with its enum value, so the
// bit a checkbox controls can never drift out of sync with its position
// in the list (unlike indexing into a plain vector<string> by position).
// AllPermissions is computed from the same list, so "grant everything"
// stays correct even if a permission is added or removed later.
short GetRequestedPermissions()
{
    static const vector<pair<string, enPermissions>> vPermissionOptions =
    {
        { "Show Client List",   pListClients },
        { "Add New Client",     pAddNewClient },
        { "Delete Client",      pDeleteClient },
        { "Update Client Info", pUpdateClient },
        { "Find Client",        pFindClient },
        { "Transactions",       pTransactions },
        { "Manage Users",       pManageUsers }
    };

    short Permissions = 0;
    short AllPermissions = 0;

    for (const pair<string, enPermissions>& Option : vPermissionOptions)
    {
        AllPermissions |= Option.second;

        if (ReadUserChoice(Option.first))
            Permissions |= Option.second;
    }

    return (Permissions == AllPermissions) ? pAll : Permissions;
}

// True if Permissions is pAll (-1, full access) or has the given
// bit set.
bool HasPermission(short Permissions, enPermissions Permission)
{
    return Permissions == pAll || (Permissions & Permission) != 0;
}

// Prompts for a user's updated password and permission set (used by
// Update User). The UserName itself isn't editable here — it's the
// key used to find the record in the first place.
sUser ChangeUserRecord(string UserName)
{
    sUser User;
    User.UserName = UserName;

    User.Password = ReadUserPassword();
    if (!ReadUserChoice("Do you want to give full access to this user"))
        User.Permissions = GetRequestedPermissions();

    else User.Permissions = -1;

    return User;
}


bool MarkClientForDeleteByAccountNumber(string AccountNumber, vector <sClient>& vClients)
{

    for (sClient& C : vClients)
    {

        if (C.AccountNumber == AccountNumber)
        {
            C.MarkForDelete = true;
            return true;
        }

    }

    return false;

}


bool MarkUserForDeleteByUserName(string UserName, vector <sUser>& vUsers)
{
    for (sUser& U : vUsers)
    {
        if (U.UserName == UserName)
        {
            U.MarkForDelete = true;
            return true;
        }
    }
    return false;
}

// Overwrites the data file with the current in-memory vector.
// Records marked for delete are skipped, effectively removing them.
vector <sClient> SaveClientsDataToFile(string FileName, vector <sClient> vClients)
{

    fstream MyFile;
    MyFile.open(FileName, ios::out);//overwrite

    string DataLine;

    if (MyFile.is_open())
    {

        for (sClient C : vClients)
        {

            if (C.MarkForDelete == false)
            {
                //we only write records that are not marked for delete.
                DataLine = ConvertClientRecordToLine(C);
                MyFile << DataLine << endl;

            }

        }

        MyFile.close();

    }

    return vClients;

}

// Overwrites Users.txt with the current in-memory vector, same
// soft-delete convention as SaveClientsDataToFile.
vector <sUser> SaveUsersDataToFile(string FileName, vector <sUser> vUsers)
{
    fstream MyFile;
    MyFile.open(FileName, ios::out);//overwrite
    string DataLine;
    if (MyFile.is_open())
    {
        for (sUser U : vUsers)
        {
            if (U.MarkForDelete == false)
            {
                //we only write records that are not marked for delete.
                DataLine = ConvertUserRecordToLine(U);
                MyFile << DataLine << endl;
            }
        }
        MyFile.close();
    }
    return vUsers;
}

void AddDataLineToFile(string FileName, string  stLine)
{
    fstream MyFile;
    MyFile.open(FileName, ios::out | ios::app);

    if (MyFile.is_open())
    {

        MyFile << stLine << endl;

        MyFile.close();
    }

}

void AddNewClient()
{
    sClient Client;
    Client = ReadNewClient();
    AddDataLineToFile(ClientsFileName, ConvertClientRecordToLine(Client));

}



// Prompts for a new user's name (re-prompting until it's unique),
// password, and permission set, then appends the record to disk.
void AddNewUser()
{
    sUser User;
    User.UserName = ReadUserName();
    vector <sUser> vUsers = LoadUsersDataFromFile(UsersFileName);

    while (FindUserByUserName(vUsers, User))
    {
        cout << "\nUser with [" << User.UserName << "] already exists, Enter another User Name? ";
        cin >> User.UserName;
    }
    User.Password = ReadUserPassword();

    if (!ReadUserChoice("Do you want to give full access to this user"))
        User.Permissions = GetRequestedPermissions();

    else User.Permissions = -1;


    AddDataLineToFile(UsersFileName, ConvertUserRecordToLine(User));

}

void AddNewClients()
{
    char AddMore = 'Y';
    do
    {
        //system("cls");
        cout << "Adding New Client:\n\n";

        AddNewClient();
        cout << "\nClient Added Successfully, do you want to add more clients? Y/N? ";


        cin >> AddMore;

    } while (toupper(AddMore) == 'Y');

}

void AddNewUsers()
{
    char AddMore = 'Y';
    do
    {
        //system("cls");
        cout << "Adding New User:\n\n";

        AddNewUser();
        cout << "\nUser Added Successfully, do you want to add more Users? Y/N? ";


        cin >> AddMore;

    } while (toupper(AddMore) == 'Y');

}

bool DeleteClientByAccountNumber(string AccountNumber, vector <sClient>& vClients)
{

    sClient Client;
    char Answer = 'n';

    if (FindClientByAccountNumber(AccountNumber, vClients, Client))
    {

        PrintClientCard(Client);

        cout << "\n\nAre you sure you want delete this client? y/n ? ";
        cin >> Answer;
        if (Answer == 'y' || Answer == 'Y')
        {
            MarkClientForDeleteByAccountNumber(AccountNumber, vClients);
            SaveClientsDataToFile(ClientsFileName, vClients);

            //Refresh Clients
            vClients = LoadClientsDataFromFile(ClientsFileName);

            cout << "\n\nClient Deleted Successfully.";
            return true;
        }

    }
    else
    {
        cout << "\nClient with Account Number (" << AccountNumber << ") is Not Found!";
        return false;
    }

    return false;
}

// Deletes a user by name, after confirmation. The "Admin" account is
// protected separately, at the screen level (see ShowDeleteUserScreen),
// before this function is even called.
bool DeleteUserByUserName(string UserName, vector <sUser>& vUsers)
{
    sUser User;
    char Answer = 'n';
    User.UserName = UserName;

    if (FindUserByUserName(vUsers, User))
    {
        PrintUserCard(User);
        cout << "\n\nAre you sure you want delete this user? y/n ? ";
        cin >> Answer;
        if (Answer == 'y' || Answer == 'Y')
        {
            MarkUserForDeleteByUserName(UserName, vUsers);
            SaveUsersDataToFile(UsersFileName, vUsers);
            //Refresh Users
            vUsers = LoadUsersDataFromFile(UsersFileName);
            cout << "\n\nUser Deleted Successfully.";
            return true;
        }
    }
    else
    {
        cout << "\nUser with User Name (" << UserName << ") is Not Found!";
        return false;
    }
    return false;
}

bool UpdateClientByAccountNumber(string AccountNumber, vector <sClient>& vClients)
{

    sClient Client;
    char Answer = 'n';

    if (FindClientByAccountNumber(AccountNumber, vClients, Client))
    {

        PrintClientCard(Client);
        cout << "\n\nAre you sure you want update this client? y/n ? ";
        cin >> Answer;
        if (Answer == 'y' || Answer == 'Y')
        {

            for (sClient& C : vClients)
            {
                if (C.AccountNumber == AccountNumber)
                {
                    C = ChangeClientRecord(AccountNumber);
                    break;
                }

            }

            SaveClientsDataToFile(ClientsFileName, vClients);

            cout << "\n\nClient Updated Successfully.";
            return true;
        }

    }
    else
    {
        cout << "\nClient with Account Number (" << AccountNumber << ") is Not Found!";
        return false;
    }

    return false;
}

// Updates a user's password/permissions via ChangeUserRecord, after
// confirmation.
bool UpdateUserByUserName(string UserName, vector <sUser>& vUsers)
{
    sUser User;
    char Answer = 'n';
    User.UserName = UserName;
    if (FindUserByUserName(vUsers, User))
    {
        PrintUserCard(User);
        cout << "\n\nAre you sure you want update this user? y/n ? ";
        cin >> Answer;
        if (Answer == 'y' || Answer == 'Y')
        {
            for (sUser& U : vUsers)
            {
                if (U.UserName == UserName)
                {
                    U = ChangeUserRecord(UserName);
                    break;
                }
            }
            SaveUsersDataToFile(UsersFileName, vUsers);
            cout << "\n\nUser Updated Successfully.";
            return true;
        }
    }
    else
    {
        cout << "\nUser with User Name (" << UserName << ") is Not Found!";
        return false;
    }
    return false;
}

// Applies a balance change to a client and saves it to disk.
// Withdrawals are implemented as a negative deposit (see
// ShowWithdrawScreen), so both transaction types share this
// same update-and-save logic.
bool DepositBalanceToClientByAccountNumber(string AccountNumber, double Amount, vector <sClient>& vClients)
{


    char Answer = 'n';


    cout << "\n\nAre you sure you want to perform this transaction? y/n ? ";
    cin >> Answer;
    if (Answer == 'y' || Answer == 'Y')
    {

        for (sClient& C : vClients)
        {
            if (C.AccountNumber == AccountNumber)
            {
                C.AccountBalance += Amount;
                SaveClientsDataToFile(ClientsFileName, vClients);
                cout << "\n\nDone Successfully. New balance is: " << C.AccountBalance;

                return true;
            }

        }


        return false;
    }

    return false;
}

string ReadClientAccountNumber()
{
    string AccountNumber = "";

    cout << "\nPlease enter AccountNumber? ";
    cin >> AccountNumber;
    return AccountNumber;

}

void ShowDeleteClientScreen(sUser User)
{
    if (!HasPermission(User.Permissions, pDeleteClient))
    {
        ShowAccessDeniedScreen();
        return;
    }

    cout << "\n-----------------------------------\n";
    cout << "\tDelete Client Screen";
    cout << "\n-----------------------------------\n";

    vector <sClient> vClients = LoadClientsDataFromFile(ClientsFileName);
    string AccountNumber = ReadClientAccountNumber();
    DeleteClientByAccountNumber(AccountNumber, vClients);

}

// "Admin" can't be deleted through this screen — checked up front,
// before DeleteUserByUserName is even called.
void ShowDeleteUserScreen()
{
    cout << "\n-----------------------------------\n";
    cout << "\tDelete User Screen";
    cout << "\n-----------------------------------\n";
    vector <sUser> vUsers = LoadUsersDataFromFile(UsersFileName);
    sUser User;
    User.UserName = ReadUserName();
    if (User.UserName == "Admin")
    {
        cout << "\nYou cannot Delete This User.";
        return;
    }
    DeleteUserByUserName(User.UserName, vUsers);
}

void ShowUpdateClientScreen(sUser User)
{
    if (!HasPermission(User.Permissions, pUpdateClient))
    {
        ShowAccessDeniedScreen();
        return;
    }

    cout << "\n-----------------------------------\n";
    cout << "\tUpdate Client Info Screen";
    cout << "\n-----------------------------------\n";

    vector <sClient> vClients = LoadClientsDataFromFile(ClientsFileName);
    string AccountNumber = ReadClientAccountNumber();
    UpdateClientByAccountNumber(AccountNumber, vClients);

}

// "Admin" is protected here too, same as ShowDeleteUserScreen. Note
// this screen updates the record inline rather than going through
// UpdateUserByUserName.
void ShowUpdateUserScreen()
{
    cout << "\n-----------------------------------\n";
    cout << "\tUpdate User Info Screen";
    cout << "\n-----------------------------------\n";
    vector <sUser> vUsers = LoadUsersDataFromFile(UsersFileName);
    sUser User;
    User.UserName = ReadUserName();
    if (User.UserName == "Admin")
    {
        cout << "\nYou cannot Update This User.";
        return;
    }
    if (FindUserByUserName(vUsers, User))
    {
        PrintUserCard(User);
        cout << "\n\nAre you sure you want update this user? y/n ? ";
        char Answer;
        cin >> Answer;
        if (Answer == 'y' || Answer == 'Y')
        {
            for (sUser& U : vUsers)
            {
                if (U.UserName == User.UserName)
                {
                    U.Password = ReadUserPassword();
                    U.Permissions = GetRequestedPermissions();
                    break;
                }
            }
            SaveUsersDataToFile(UsersFileName, vUsers);
            cout << "\n\nUser Updated Successfully.";
        }
    }
    else
    {
        cout << "\nUser with User Name (" << User.UserName << ") is Not Found!";
    }
}



void ShowAddNewClientsScreen(sUser User)
{
    if (!HasPermission(User.Permissions, pAddNewClient))
    {
        ShowAccessDeniedScreen();
        return;
    }

    cout << "\n-----------------------------------\n";
    cout << "\tAdd New Clients Screen";
    cout << "\n-----------------------------------\n";

    AddNewClients();

}

void ShowAddNewUserScreen()
{
    cout << "\n-----------------------------------\n";
    cout << "\tAdd New User Screen";
    cout << "\n-----------------------------------\n";

    AddNewUsers();
}

void ShowFindClientScreen(sUser User)
{
    if (!HasPermission(User.Permissions, pFindClient))
    {
        ShowAccessDeniedScreen();
        return;
    }

    cout << "\n-----------------------------------\n";
    cout << "\tFind Client Screen";
    cout << "\n-----------------------------------\n";

    vector <sClient> vClients = LoadClientsDataFromFile(ClientsFileName);
    sClient Client;
    string AccountNumber = ReadClientAccountNumber();
    if (FindClientByAccountNumber(AccountNumber, vClients, Client))
        PrintClientCard(Client);
    else
        cout << "\nClient with Account Number[" << AccountNumber << "] is not found!";

}

void ShowFindUserScreen()
{
    cout << "\n-----------------------------------\n";
    cout << "\tFind User Screen";
    cout << "\n-----------------------------------\n";
    vector <sUser> vUsers = LoadUsersDataFromFile(UsersFileName);
    sUser User;
    User.UserName = ReadUserName();
    if (FindUserByUserName(vUsers, User))
        PrintUserCard(User);
    else
        cout << "\nUser with User Name[" << User.UserName << "] is not found!";
}

void ShowDepositScreen()
{
    cout << "\n-----------------------------------\n";
    cout << "\tDeposit Screen";
    cout << "\n-----------------------------------\n";


    sClient Client;

    vector <sClient> vClients = LoadClientsDataFromFile(ClientsFileName);
    string AccountNumber = ReadClientAccountNumber();


    while (!FindClientByAccountNumber(AccountNumber, vClients, Client))
    {
        cout << "\nClient with [" << AccountNumber << "] does not exist.\n";
        AccountNumber = ReadClientAccountNumber();
    }


    PrintClientCard(Client);

    double Amount = ReadValidatedDouble("\nPlease enter deposit amount? ");

    DepositBalanceToClientByAccountNumber(AccountNumber, Amount, vClients);

}

void ShowWithdrawScreen()
{
    cout << "\n-----------------------------------\n";
    cout << "\tWithdraw Screen";
    cout << "\n-----------------------------------\n";

    sClient Client;

    vector <sClient> vClients = LoadClientsDataFromFile(ClientsFileName);
    string AccountNumber = ReadClientAccountNumber();


    while (!FindClientByAccountNumber(AccountNumber, vClients, Client))
    {
        cout << "\nClient with [" << AccountNumber << "] does not exist.\n";
        AccountNumber = ReadClientAccountNumber();
    }

    PrintClientCard(Client);

    double Amount = ReadValidatedDouble("\nPlease enter withdraw amount? ");

    //Validate that the amount does not exceeds the balance
    while (Amount > Client.AccountBalance)
    {
        cout << "\nAmount Exceeds the balance, you can withdraw up to : " << Client.AccountBalance << endl;
        Amount = ReadValidatedDouble("Please enter another amount? ");
    }

    // Withdrawal = deposit of a negative amount; reuses the same
    // balance-update and save logic as a deposit.
    DepositBalanceToClientByAccountNumber(AccountNumber, Amount * -1, vClients);

}

void ShowTotalBalancesScreen()
{

    ShowTotalBalances();

}

enum enTransactionsMenuOptions
{
    eDeposit = 1,
    eWithdraw = 2,
    eShowTotalBalance = 3,
    eShowMainMenu = 4
};

enum enMainMenuOptions
{
    eListClients = 1,
    eAddNewClient = 2,
    eDeleteClient = 3,
    eUpdateClient = 4,
    eFindClient = 5,
    eShowTransactionsMenu = 6,
    eShowManageUsersMenu = 7,
    eLogout = 8
};

enum enManageUserMenuOptions
{
    eListUser = 1,
    eAddNewUser = 2,
    eDeleteUser = 3,
    eUpdateUser = 4,
    eFindUser = 5,
    eMainMenu = 6,
};
// Note: navigation between menus uses mutual recursion (each screen
// calls back into ShowMainMenu/ShowTransactionsMenu) rather than a
// loop, so the call stack grows with each menu visit for as long as
// the program runs. This is fine for a simple console app of this size.

void GoBackToMainMenu(sUser User)
{
    cout << "\n\nPress any key to go back to Main Menu...";
    system("pause>0");
    ShowMainMenu(User);
}

void GoBackToTransactionsMenu(sUser User)
{
    cout << "\n\nPress any key to go back to Transactions Menu...";
    system("pause>0");
    ShowTransactionsMenu(User);
}

void GoBackToManageUsersMenu(sUser User)
{
    cout << "\n\nPress any key to go back to Manage Users Menu...";
    system("pause>0");
    ShowManageUsersMenu(User);
}

short ReadTransactionsMenuOption()
{
    return ReadValidatedShort("Choose what do you want to do? [1 to 4]? ");
}

short ReadManageUsersMenuOption()
{
    return ReadValidatedShort("Choose what do you want to do? [1 to 6]? ");
}

void PerformTransactionsMenuOption(enTransactionsMenuOptions TransactionMenuOption, sUser User)
{
    switch (TransactionMenuOption)
    {
    case enTransactionsMenuOptions::eDeposit:
    {
        system("cls");
        ShowDepositScreen();
        GoBackToTransactionsMenu(User);
        break;
    }

    case enTransactionsMenuOptions::eWithdraw:
    {
        system("cls");
        ShowWithdrawScreen();
        GoBackToTransactionsMenu(User);
        break;
    }


    case enTransactionsMenuOptions::eShowTotalBalance:
    {
        system("cls");
        ShowTotalBalancesScreen();
        GoBackToTransactionsMenu(User);
        break;
    }


    case enTransactionsMenuOptions::eShowMainMenu:
    {

        ShowMainMenu(User);
        break;

    }

    default:
        cout << "\nInvalid choice, please enter a number between 1 and 4.";
        GoBackToTransactionsMenu(User);
        break;
    }

}

void PerformManageUsersMenuOption(enManageUserMenuOptions ManageUsersMenuOption, sUser User)
{
    switch (ManageUsersMenuOption)
    {
    case enManageUserMenuOptions::eListUser:
    {
        system("cls");
        ShowAllUsersScreen();
        GoBackToManageUsersMenu(User);
        break;
    }
    case enManageUserMenuOptions::eAddNewUser:
    {
        system("cls");
        ShowAddNewUserScreen();
        GoBackToManageUsersMenu(User);
        break;
    }
    case enManageUserMenuOptions::eDeleteUser:
    {
        system("cls");
        ShowDeleteUserScreen();
        GoBackToManageUsersMenu(User);
        break;
    }
    case enManageUserMenuOptions::eUpdateUser:
    {
        system("cls");
        ShowUpdateUserScreen();
        GoBackToManageUsersMenu(User);
        break;
    }
    case enManageUserMenuOptions::eFindUser:
    {
        system("cls");
        ShowFindUserScreen();
        GoBackToManageUsersMenu(User);
        break;
    }
    case enManageUserMenuOptions::eMainMenu:
    {
        ShowMainMenu(User);
        break;
    }
    default:
        cout << "\nInvalid choice, please enter a number between 1 and 6.";
        GoBackToManageUsersMenu(User);
        break;
    }
}

void ShowTransactionsMenu(sUser User)
{
    if (!HasPermission(User.Permissions, pTransactions))
    {
        ShowAccessDeniedScreen();
        return;
    }

    system("cls");
    cout << "===========================================\n";
    cout << "\t\tTransactions Menu Screen\n";
    cout << "===========================================\n";
    cout << "\t[1] Deposit.\n";
    cout << "\t[2] Withdraw.\n";
    cout << "\t[3] Total Balances.\n";
    cout << "\t[4] Main Menu.\n";
    cout << "===========================================\n";
    PerformTransactionsMenuOption((enTransactionsMenuOptions)ReadTransactionsMenuOption(), User);
}

// Guards entry to the whole Manage Users menu. Individual actions
// inside it (Add/Delete/Update/Find User) are not separately
// permission-gated — anyone who can reach this menu can do all of
// them. A finer-grained per-action check would mirror the client
// screens' pattern above, but wasn't needed for this project's scope.
void ShowManageUsersMenu(sUser User)
{
    if (!HasPermission(User.Permissions, pManageUsers))
    {
        ShowAccessDeniedScreen();
        return;
    }

    system("cls");
    cout << "===========================================\n";
    cout << "\t\tManage Users Menu Screen\n";
    cout << "===========================================\n";
    cout << "\t[1] List Users.\n";
    cout << "\t[2] Add New User.\n";
    cout << "\t[3] Delete User.\n";
    cout << "\t[4] Update User.\n";
    cout << "\t[5] Find User.\n";
    cout << "\t[6] Main Menu.\n";
    cout << "===========================================\n";
    PerformManageUsersMenuOption((enManageUserMenuOptions)ReadManageUsersMenuOption(), User);
}

short ReadMainMenuOption()
{
    return ReadValidatedShort("Choose what do you want to do? [1 to 8]? ");
}

void PerformMainMenuOption(enMainMenuOptions MainMenuOption, sUser User)
{
    switch (MainMenuOption)
    {
    case enMainMenuOptions::eListClients:
        system("cls");
        ShowAllClientsScreen(User);
        GoBackToMainMenu(User);
        break;

    case enMainMenuOptions::eAddNewClient:
        system("cls");
        ShowAddNewClientsScreen(User);
        GoBackToMainMenu(User);
        break;

    case enMainMenuOptions::eDeleteClient:
        system("cls");
        ShowDeleteClientScreen(User);
        GoBackToMainMenu(User);
        break;

    case enMainMenuOptions::eUpdateClient:
        system("cls");
        ShowUpdateClientScreen(User);
        GoBackToMainMenu(User);
        break;

    case enMainMenuOptions::eFindClient:
        system("cls");
        ShowFindClientScreen(User);
        GoBackToMainMenu(User);
        break;

    case enMainMenuOptions::eShowTransactionsMenu:
        system("cls");
        ShowTransactionsMenu(User);
        GoBackToMainMenu(User);
        break;

    case enMainMenuOptions::eShowManageUsersMenu:
        system("cls");
        ShowManageUsersMenu(User);
        GoBackToMainMenu(User);
        break;

    case enMainMenuOptions::eLogout:
        system("cls");
        ShowLoginScreen();
        break;

    default:
        cout << "\nInvalid choice, please enter a number between 1 and 8.";
        GoBackToMainMenu(User);
        break;
    }

}

void ShowMainMenu(sUser User)
{
    system("cls");
    cout << "===========================================\n";
    cout << "\t\tMain Menu Screen\n";
    cout << "===========================================\n";
    cout << "\t[1] Show Client List.\n";
    cout << "\t[2] Add New Client.\n";
    cout << "\t[3] Delete Client.\n";
    cout << "\t[4] Update Client Info.\n";
    cout << "\t[5] Find Client.\n";
    cout << "\t[6] Transactions.\n";
    cout << "\t[7] Manage Users.\n";
    cout << "\t[8] Logout.\n";
    cout << "===========================================\n";
    PerformMainMenuOption((enMainMenuOptions)ReadMainMenuOption(), User);
}

// Prints the invalid-credentials message, then immediately blocks on
// the next username prompt (via ReadNewUser) so the user has time to
// read it before the next loop iteration clears the screen.
void ShowLoginScreen()
{
    vector <sUser> vUsers = LoadUsersDataFromFile(UsersFileName);
    bool LoginFailed = false;

    while (true)
    {
        system("cls");
        cout << "\n-----------------------------------\n";
        cout << "\tLogin Screen";
        cout << "\n-----------------------------------\n";

        if (LoginFailed)
            cout << "Invalid Username/Password!\n";

        sUser User = ReadNewUser();
        if (FindUserByUserNameAndPassword(vUsers, User))
        {
            ShowMainMenu(User);
            break;
        }

        LoginFailed = true;
    }
}


int main()
{
    ShowLoginScreen();
    system("pause>0");
    return 0;
}