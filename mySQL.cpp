// Harrison Clark and Parker Dawes
// SQL set up for a database connection with the mysql library for c++
// Compile w/ g++ filename -o name /usr/include/mariadb/mysql.h -lmariadclient

// Used https://www.youtube.com/watch?v=WlEFQPvKUPo

#include <iostream>
#include <mariadb/mysql.h>

struct connection_details{
  const char *server, *user, *password, *database;
}

MYSQL* mysql_connection_setup(struct connection_details mysql_details) {
  MYSQL *connection = mysql_init(NULL)

  if (!mysql_real_connect(connection, mysql_details.server, mysql_details.user, mysql_details.password, mysql_details.database, 0, NULL, 0) {
    std::cout << "Connection Error: " << mysql_error(connection) << '\n';
    exit(1);
  }

  return connection;
}  

MYSQL_RES* mysql_execute_query(MYSQL *connection, const char *sql_query) {
  if (mysql_query(connection, sql_query)) {
    std::cout << "MYSQL Query Error: " << mysql_error(connection) << '\n';
    exit(1);
  }

  return mysql_use_result(connection);
}

int main(int argc, char* argv[]) {
  MYSQL* con;
  MYSQL* res;
  MYSQL row;

  struct connection_details mysql;
  mysql.server = "localhost";
  mysql.user = "usr";
  mysql.password = "qwerty";
  mysql.database = "secret";

  con = mysql_connection_setup(mysql);
  res = mysql_execute_query(con, "select * from tlbUsrs");

  while (row = mysql_fetch(res) != NULL) {
    std::cout << row[0] << "\n\n";
  }

  mysql_free_result(res);
  mysql_close(con);

  return 0;
}
