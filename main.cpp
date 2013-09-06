#include <iostream>

#include "Tree.h"

using namespace std;

int main()
{
    try {
        int a;
        string op;
        BTree t("tree");
        while(true)
        {
            cout << "enter op:" << endl;
            cin >> op;
            if(op == "close")
                return 0;
            else if(op == "list")
                t.preorder();
            else if(op != "insert" && op != "find" && op != "delete")
                cout << "wrong op" << endl;
            else
            {
                cout << "enter key: " << endl;
                cin >> a;
                if(op == "insert")
                    t.insert_key(a);
                else if (op == "find") {
                    if(t.find_key(a)) cout << "found" << endl;
                    else cout << "not found" << endl;
                }
                else {
                    if(t.delete_key(a)) cout << "deleted" << endl;
                    else cout << "not found" << endl;
                }
            }
        }
    }
    catch(excCorrupted e)
    {
        cout << e.what() << endl;
    }
}
