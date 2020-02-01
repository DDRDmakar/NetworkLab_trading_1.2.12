
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
 * Команды администратора:
 *     add <price> <name> - Добавить лот со стартовой ценой
 *     disconnect <name> - Разорвать соединение с участником торгов
 *     finish - завершение торгов
 * Команды клиента:
 *     list - получить текущий список лотов
 *     price <newprice> <name> - Повышение цены лота
 */


#include <iostream>

#include "server.hpp"
#include "tools.hpp"


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
	char buffer[BUFFER_LEN];
	int *intvec = (int*)buffer;
	
	while (cmd != "q")
	{
		std::getline(std::cin, cmd);
		auto tokens = tokenize_string(cmd); // Args vector
		
		if (false) {}
		else
		{
			servak->command("", tokens, buffer, BUFFER_LEN);
			if (intvec[0] == MTYPE_DISCONNECT) break;
			else
			{
				if (intvec[0] == MTYPE_TEXT) std::cout << buffer + sizeof(int) << std::endl;
			}
		}
	}
}
