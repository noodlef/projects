#include"DBPool.h"
#include<iostream>
#include<string>
#include<stdio.h>

using namespace std;

int main(){
    CDBManager* g_db_manager = CDBManager::getInstance();
    CDBConn* conn = g_db_manager->GetDBConn("teamtalk_master");
    while(true){
        string query ;//"show tables;";
        cin >> query;
        cout << query << endl;
        CResultSet* result = conn->ExecuteQuery(query.c_str());
        map<string, int>& keys = result->MapKey();
        /*for(size_t i = 0; i < keys.size(); i++)
           cout << keys[(int)i] << "        " ;
        cout << endl;*/
        if(!result) continue;
        while(result->Next()){
            for(size_t i = 0; i < keys.size(); i++){
                cout << result->GetRow()[(int)i] << "          ";                        
            }
            cout << endl;
        }
        delete result;
    }
    delete g_db_manager;
}
