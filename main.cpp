
/**
 * Задание 1.2.12 (Система торгов)
 * СЕРВЕР
 * 
 * На торги выставляются лоты, имеющие начальную стоимость. 
 * Участники торгов могут повышать стоимость лота. Распорядитель
 * может прекратить торги. При окончании торгов всем участникам рассылаются 
 * результаты торгов.
 * 
 * Функции
 *     1) Прослушивание определённого порта
 *     2) Обработка запросов на подключение по этому порту от клиентов 
 *     системы торгов (как распорядитель или участник торгов)
 *     3) Поддержка одновременной работы нескольких клиентов системы
 *     торгов через механизм нитей
 *     4) От участников торгов: прием запросов на передачу списка лотов
 *     5) От участников торгов: прием запросов на повышение стоимости лота
 *     6) От распорядителя: прием запроса на добавление нового лота с пер-
 *     воначальной стоимостью
 *     7) От распорядителя: прием запроса об окончании торгов
 *     8) Осуществление добавления лота, учет повышения стоимости лота
 *     участниками, завершение торгов и рассылка результатов торгов всем
 *     участникам
 *     9) Обработка запроса на отключение клиента
 *     10) Принудительное отключение клиента
 */

/*
 * Команды сервера:
 * Команды клиента:
 *     list - получить текущий список лотов
 *     add <price> <name> - Добавить лот со стартовой ценой
 *     price <newprice> <name> - Повышение цены лота
 *     disconnect <name> - Разорвать соединение с участником торгов
 */


#include <iostream>

#include "server.hpp"


void cli_routine(Server *servak);

int main(int argc, char** argv)
{
	unsigned int server_port = 6000;
	
	try
	{
		Server servak(server_port, 10);
		cli_routine(&servak);
	}
	catch (Exception &e)
	{
		std::cout << e.what() << std::endl;
	}
	
	return 0;
}

// TOKENIZE STRING
std::vector<std::string> tokenize_string(const std::string &str, char delim = ' ')
{
	std::vector<std::string> cont;
	std::size_t current, previous= 0;
	current = str.find(delim);
	while (current != std::string::npos)
	{
		cont.push_back(str.substr(previous, current - previous));
		previous = current + 1;
		current = str.find(delim, previous);
	}
	cont.push_back(str.substr(previous, current - previous));
	
	return cont;
}

// LISTENING CONSOLE INPUT
void cli_routine(Server *servak)
{
	printf("CLI routine start\n");
	
	//char buffer[BUFFER_LEN];
	//bzero(buffer, BUFFER_LEN);
	
	// MUTEX
	//pthread_mutex_t     mut;
	//pthread_mutexattr_t matr;
	
	//pthread_mutexattr_init(&matr);
	//pthread_mutexattr_setpshared(&matr, PTHREAD_PROCESS_PRIVATE);
	//pthread_mutex_init(&mut, &matr);
	
	std::string cmd;
	
	while (cmd != "exit")
	{
		std::getline(std::cin, cmd);
		auto args = tokenize_string(cmd);
		
		if (args[0] == "disconnect" && args.size() == 2)
		{
			servak->disconnect(args[1]);
		}
		
		/*printf("> ");
		bzero(buffer, BUFFER_LEN);
		
		// WAITING HERE
		fgets(buffer, BUFFER_LEN, stdin);
		
		pthread_mutex_lock(&mut);
		
		 // Show thread list
		if (!strcmp(buffer, "list\n"))
		{
			print_threadlist();
		}
		
		// Close connection with clinet N
		if ('0' <= buffer[0] && buffer[0] <= '9')
		{
			int thread_idx = atoi(buffer);
			if (
				thread_idx < 0                         ||
				thread_idx >= MAX_CLIENT_THREAD_NUMBER ||
				clients[thread_idx].state != TSTATE_WORKING
			)
			{
				error_proceed("Error - client does not exist");
				pthread_mutex_unlock(&mut);
				continue;
			}
			disconnect_client(thread_idx);
			stop_client(thread_idx);
		}
		
		// Show help text
		if (!strcmp(buffer, "help\n"))
		{
			printf(
				"*\n"
				"* Список доступных команд:\n"
				"*     help - показать это сообщение\n"
				"*     list - показать список клиентов и их статус\n"
				"*     <N>  - разорвать соединение с\n"
				"*            клиентом номер N\n"
				"*     exit - завершить работу сервера\n"
				"*\n"
			);
		}
		
		pthread_mutex_unlock(&mut);*/
	}
	/*
	int s = *((int*)arg);
	
	printf("Server is shutting down\n");
	
	if (shutdown(s, SHUT_RDWR)) error_out("TCP connection shutdown failed");
	if (close(s)) error_out("Server socket closing failed");
	
	printf("CLI thread finish\n");
	*/
}
