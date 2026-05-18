void Library::login() {
    system("clear");
    cout << "===========================================" << endl;
    cout << "               User Login System           " << endl;
    cout << "===========================================" << endl;
    cout << "Enter your name: ";
    string name;
    getline(cin, name);
    
    // 檢查是否為訪客並設定 isGuest 旗標
    bool isGuest = false;
    if (name.empty() || name == "Guest") {
        name = "Guest";
        isGuest = true;
    }
    
    // 將 isGuest 傳入建構子
    currentUser = new User(name, isGuest); 
    cout << "\nWelcome, " << currentUser->name << "! Press Enter to enter library...";
    getchar();
}