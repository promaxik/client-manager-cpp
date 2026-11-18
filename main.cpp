#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <exception> 

using namespace std;

class ClientManager {
private:
    string connection_string;

public:
    ClientManager(string conn_str) : connection_string(conn_str) {}

    // 1. Ñîçäàíèå òàáëèö
    void createTables() {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);

            // Òàáëèöà êëèåíòîâ
            tx.exec("CREATE TABLE IF NOT EXISTS clients ("
                "id SERIAL PRIMARY KEY,"
                "first_name VARCHAR(50) NOT NULL,"
                "last_name VARCHAR(50) NOT NULL,"
                "email VARCHAR(100) UNIQUE NOT NULL);");

            // Òàáëèöà òåëåôîíîâ (ñâÿçü Îäèí-êî-Ìíîãèì ñ êàñêàäíûì óäàëåíèåì)
            tx.exec("CREATE TABLE IF NOT EXISTS phones ("
                "id SERIAL PRIMARY KEY,"
                "client_id INTEGER NOT NULL REFERENCES clients(id) ON DELETE CASCADE,"
                "phone_number VARCHAR(20) NOT NULL);");
            tx.commit();
            cout << "[OK] Òàáëèöû óñïåøíî ñîçäàíû." << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Îøèáêà ïðè ñîçäàíèè òàáëèö: " << e.what() << endl;
        }
    }

    // 2. Äîáàâëåíèå êëèåíòà (èñïîëüçóåì exec_params äëÿ çàùèòû îò SQL-èíúåêöèé)
    int addClient(const string& first_name, const string& last_name, const string& email) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            // Áåçîïàñíàÿ ïåðåäà÷à ïàðàìåòðîâ ÷åðåç $1, $2, $3
            pqxx::result res = tx.exec_params(
                "INSERT INTO clients (first_name, last_name, email) VALUES ($1, $2, $3) RETURNING id;",
                first_name, last_name, email);
            tx.commit();
            int new_id = res[0][0].as<int>();
            cout << "[OK] Êëèåíò äîáàâëåí. ID: " << new_id << endl;
            return new_id;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Îøèáêà ïðè äîáàâëåíèè êëèåíòà: " << e.what() << endl;
            return -1;
        }
    }

    // 3. Äîáàâëåíèå òåëåôîíà
    void addPhone(int client_id, const string& phone) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            tx.exec_params("INSERT INTO phones (client_id, phone_number) VALUES ($1, $2);", client_id, phone);
            tx.commit();
            cout << "[OK] Òåëåôîí äîáàâëåí êëèåíòó ID " << client_id << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Îøèáêà ïðè äîáàâëåíèè òåëåôîíà: " << e.what() << endl;
        }
    }

    // 4. Èçìåíåíèå äàííûõ êëèåíòà
    void updateClient(int client_id, const string& new_first, const string& new_last, const string& new_email) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            tx.exec_params("UPDATE clients SET first_name = $1, last_name = $2, email = $3 WHERE id = $4;",
                new_first, new_last, new_email, client_id);
            tx.commit();
            cout << "[OK] Äàííûå êëèåíòà ID " << client_id << " îáíîâëåíû." << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Îøèáêà ïðè îáíîâëåíèè: " << e.what() << endl;
        }
    }

    // 5. Óäàëåíèå òåëåôîíà
    void deletePhone(int client_id, const string& phone) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            tx.exec_params("DELETE FROM phones WHERE client_id = $1 AND phone_number = $2;", client_id, phone);
            tx.commit();
            cout << "[OK] Òåëåôîí " << phone << " óäàëåí ó êëèåíòà ID " << client_id << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Îøèáêà ïðè óäàëåíèè òåëåôîíà: " << e.what() << endl;
        }
    }

    // 6. Óäàëåíèå êëèåíòà (òåëåôîíû óäàëÿòñÿ àâòîìàòè÷åñêè áëàãîäàðÿ ON DELETE CASCADE)
    void deleteClient(int client_id) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            tx.exec_params("DELETE FROM clients WHERE id = $1;", client_id);
            tx.commit();
            cout << "[OK] Êëèåíò ID " << client_id << " è âñå åãî òåëåôîíû óäàëåíû." << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Îøèáêà ïðè óäàëåíèè êëèåíòà: " << e.what() << endl;
        }
    }

    // 7. Ïîèñê êëèåíòà ïî ëþáûì äàííûì
    void findClient(const string& query) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);

            // Ïîèñê ïî èìåíè, ôàìèëèè, email èëè òåëåôîíó. ILIKE - ðåãèñòðîíåçàâèñèìûé ïîèñê.
            string sql = "SELECT DISTINCT c.id, c.first_name, c.last_name, c.email "
                "FROM clients c LEFT JOIN phones p ON c.id = p.client_id "
                "WHERE c.first_name ILIKE $1 OR c.last_name ILIKE $1 OR c.email ILIKE $1 OR p.phone_number ILIKE $1;";

            string search = "%" + query + "%";
            pqxx::result res = tx.exec_params(sql, search);

            if (res.empty()) {
                cout << "Êëèåíò ïî çàïðîñó '" << query << "' íå íàéäåí." << endl;
                return;
            }

            cout << "--- Ðåçóëüòàòû ïîèñêà: '" << query << "' ---" << endl;
            for (auto row : res) {
                cout << "ID: " << row["id"].as<int>()
                    << " | " << row["first_name"].as<string>()
                    << " " << row["last_name"].as<string>()
                    << " | Email: " << row["email"].as<string>() << endl;
            }
        }
        catch (const exception& e) {
            cerr << "[ERROR] Îøèáêà ïðè ïîèñêå: " << e.what() << endl;
        }
    }
};

int main() {
    string conn_str = "dbname=test_clients user=postgres password=maxMAX1965 host=localhost port=5432";

    ClientManager manager(conn_str);

    cout << "=== ÄÅÌÎÍÑÒÐÀÖÈß ÐÀÁÎÒÛ ===" << endl;

    // 1. Ñîçäàåì òàáëèöû
    manager.createTables();

    // 2. Äîáàâëÿåì êëèåíòîâ
    int id1 = manager.addClient("Èâàí", "Èâàíîâ", "ivan@mail.ru");
    int id2 = manager.addClient("Àííà", "Ñìèðíîâà", "anna@yandex.ru");

    // 3. Äîáàâëÿåì òåëåôîíû
    manager.addPhone(id1, "+79991112233");
    manager.addPhone(id1, "+78887776655"); // Ó Èâàíà äâà òåëåôîíà
    manager.addPhone(id2, "+79123456789"); // Ó Àííû îäèí

    // 4. Èçìåíÿåì äàííûå
    manager.updateClient(id2, "Àííà", "Ïåòðîâà", "anna_new@yandex.ru");

    // 5. Óäàëÿåì îäèí òåëåôîí
    manager.deletePhone(id1, "+78887776655");

    // 7. Èùåì êëèåíòîâ (äî óäàëåíèÿ)
    cout << "\n--- Ïîèñê êëèåíòîâ ---" << endl;
    manager.findClient("Èâàí");       // Ïî èìåíè
    manager.findClient("+7999");      // Ïî ÷àñòè òåëåôîíà

    // 6. Óäàëÿåì êëèåíòà
    manager.deleteClient(id1);

    // Ïðîâåðÿåì, ÷òî Èâàí óäàëèëñÿ âìåñòå ñ òåëåôîíàìè
    cout << "\n--- Ïîèñê óäàëåííîãî êëèåíòà ---" << endl;
    manager.findClient("Èâàí");

    return 0;
}
