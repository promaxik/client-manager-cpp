#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <exception> // Добавлено для строгого соответствия std::exception

using namespace std;

class ClientManager {
private:
    string connection_string;

public:
    ClientManager(string conn_str) : connection_string(conn_str) {}

    // 1. Создание таблиц
    void createTables() {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);

            // Таблица клиентов
            tx.exec("CREATE TABLE IF NOT EXISTS clients ("
                "id SERIAL PRIMARY KEY,"
                "first_name VARCHAR(50) NOT NULL,"
                "last_name VARCHAR(50) NOT NULL,"
                "email VARCHAR(100) UNIQUE NOT NULL);");

            // Таблица телефонов (связь Один-ко-Многим с каскадным удалением)
            tx.exec("CREATE TABLE IF NOT EXISTS phones ("
                "id SERIAL PRIMARY KEY,"
                "client_id INTEGER NOT NULL REFERENCES clients(id) ON DELETE CASCADE,"
                "phone_number VARCHAR(20) NOT NULL);");
            tx.commit();
            cout << "[OK] Таблицы успешно созданы." << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Ошибка при создании таблиц: " << e.what() << endl;
        }
    }

    // 2. Добавление клиента (используем exec_params для защиты от SQL-инъекций)
    int addClient(const string& first_name, const string& last_name, const string& email) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            // Безопасная передача параметров через $1, $2, $3
            pqxx::result res = tx.exec_params(
                "INSERT INTO clients (first_name, last_name, email) VALUES ($1, $2, $3) RETURNING id;",
                first_name, last_name, email);
            tx.commit();
            int new_id = res[0][0].as<int>();
            cout << "[OK] Клиент добавлен. ID: " << new_id << endl;
            return new_id;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Ошибка при добавлении клиента: " << e.what() << endl;
            return -1;
        }
    }

    // 3. Добавление телефона
    void addPhone(int client_id, const string& phone) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            tx.exec_params("INSERT INTO phones (client_id, phone_number) VALUES ($1, $2);", client_id, phone);
            tx.commit();
            cout << "[OK] Телефон добавлен клиенту ID " << client_id << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Ошибка при добавлении телефона: " << e.what() << endl;
        }
    }

    // 4. Изменение данных клиента
    void updateClient(int client_id, const string& new_first, const string& new_last, const string& new_email) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            tx.exec_params("UPDATE clients SET first_name = $1, last_name = $2, email = $3 WHERE id = $4;",
                new_first, new_last, new_email, client_id);
            tx.commit();
            cout << "[OK] Данные клиента ID " << client_id << " обновлены." << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Ошибка при обновлении: " << e.what() << endl;
        }
    }

    // 5. Удаление телефона
    void deletePhone(int client_id, const string& phone) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            tx.exec_params("DELETE FROM phones WHERE client_id = $1 AND phone_number = $2;", client_id, phone);
            tx.commit();
            cout << "[OK] Телефон " << phone << " удален у клиента ID " << client_id << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Ошибка при удалении телефона: " << e.what() << endl;
        }
    }

    // 6. Удаление клиента (телефоны удалятся автоматически благодаря ON DELETE CASCADE)
    void deleteClient(int client_id) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);
            tx.exec_params("DELETE FROM clients WHERE id = $1;", client_id);
            tx.commit();
            cout << "[OK] Клиент ID " << client_id << " и все его телефоны удалены." << endl;
        }
        catch (const exception& e) {
            cerr << "[ERROR] Ошибка при удалении клиента: " << e.what() << endl;
        }
    }

    // 7. Поиск клиента по любым данным
    void findClient(const string& query) {
        try {
            pqxx::connection conn(connection_string);
            pqxx::work tx(conn);

            // Поиск по имени, фамилии, email или телефону. ILIKE - регистронезависимый поиск.
            string sql = "SELECT DISTINCT c.id, c.first_name, c.last_name, c.email "
                "FROM clients c LEFT JOIN phones p ON c.id = p.client_id "
                "WHERE c.first_name ILIKE $1 OR c.last_name ILIKE $1 OR c.email ILIKE $1 OR p.phone_number ILIKE $1;";

            string search = "%" + query + "%";
            pqxx::result res = tx.exec_params(sql, search);

            if (res.empty()) {
                cout << "Клиент по запросу '" << query << "' не найден." << endl;
                return;
            }

            cout << "--- Результаты поиска: '" << query << "' ---" << endl;
            for (auto row : res) {
                cout << "ID: " << row["id"].as<int>()
                    << " | " << row["first_name"].as<string>()
                    << " " << row["last_name"].as<string>()
                    << " | Email: " << row["email"].as<string>() << endl;
            }
        }
        catch (const exception& e) {
            cerr << "[ERROR] Ошибка при поиске: " << e.what() << endl;
        }
    }
};

int main() {
    string conn_str = "dbname=test_clients user=postgres password=maxMAX1965 host=localhost port=5432";

    ClientManager manager(conn_str);

    cout << "=== ДЕМОНСТРАЦИЯ РАБОТЫ ===" << endl;

    // 1. Создаем таблицы
    manager.createTables();

    // 2. Добавляем клиентов
    int id1 = manager.addClient("Иван", "Иванов", "ivan@mail.ru");
    int id2 = manager.addClient("Анна", "Смирнова", "anna@yandex.ru");

    // 3. Добавляем телефоны
    manager.addPhone(id1, "+79991112233");
    manager.addPhone(id1, "+78887776655"); // У Ивана два телефона
    manager.addPhone(id2, "+79123456789"); // У Анны один

    // 4. Изменяем данные
    manager.updateClient(id2, "Анна", "Петрова", "anna_new@yandex.ru");

    // 5. Удаляем один телефон
    manager.deletePhone(id1, "+78887776655");

    // 7. Ищем клиентов (до удаления)
    cout << "\n--- Поиск клиентов ---" << endl;
    manager.findClient("Иван");       // По имени
    manager.findClient("+7999");      // По части телефона

    // 6. Удаляем клиента
    manager.deleteClient(id1);

    // Проверяем, что Иван удалился вместе с телефонами
    cout << "\n--- Поиск удаленного клиента ---" << endl;
    manager.findClient("Иван");

    return 0;
}