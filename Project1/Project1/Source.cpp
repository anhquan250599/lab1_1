#include <windows.h>
#include <tchar.h>
#include <cmath>
#include <fstream>
#define M_PI 3.14

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
TCHAR WinName[] = _T("MainFrame");

int APIENTRY WinMain(HINSTANCE This, HINSTANCE Prev, LPSTR cmd, int mode)
{
	HWND hWnd; //Дескриптор главного окна программы
	MSG msg; //Структура для хранения сообщений
	WNDCLASS wc; //Класс окна
	//Определение класса окна
	wc.hInstance = This;
	wc.lpszClassName = WinName; //Имя класса окна
	wc.lpfnWndProc = WndProc; //Функция окна
	wc.style = CS_HREDRAW | CS_VREDRAW; //Стиль окна
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); //Стандартная иконка
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); //Стандарный курсор
	wc.lpszMenuName = NULL; // Нет меню
	wc.cbClsExtra = 0; //Нет дополнительных данных класса
	wc.cbWndExtra = 0; //Нет дополнительных данных класса
	// Заполнение окна белым цветом
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); //установка цвета фона
	if (!RegisterClass(&wc)) return 0; //Регистрация класса окна
	//Создание окна
	hWnd = CreateWindow(WinName,//Имя класса окна
		_T("Каркас Windows-приложения"),// Заголовок окна
		WS_OVERLAPPEDWINDOW, //Стиль окна
		CW_USEDEFAULT, // X
		CW_USEDEFAULT, // Y
		CW_USEDEFAULT, // width
		CW_USEDEFAULT, //Height
		HWND_DESKTOP, // Дескриптор родительского окна
		NULL, // Нет меню
		This, // Дескриптор приложения
		NULL); //Дополнительной информации нет
	ShowWindow(hWnd, mode); //Показать окно
	//Цикл обработки сообщений
	while (GetMessage(&msg, NULL, 0, 0))//цикл получения сообщений
	{
		TranslateMessage(&msg); //Функция трансляции кодов нажатиой клавиши
		DispatchMessageW(&msg); // Посылает сообщение функции WndProc()
	} return 0; //при положительном завершении программы в главную функцию возвращается 0
}

static int sx, sy;
const int MAX = 10;
const int SCALE = 1000; //размер логического окна
const int MARK = 4; //размер прямоугольного маркера точек
void DcInLp(POINT& point)
{
	point.x = point.x * SCALE / sx;
	point.y = SCALE - point.y * SCALE / sy;
}
void transform(HDC& hdc)
{
	SetMapMode(hdc, MM_ANISOTROPIC); //Устанавливаем локальную систему
	SetWindowExtEx(hdc, SCALE, -SCALE, NULL); //координат 1000х1000 с центром в
	SetViewportExtEx(hdc, sx, sy, NULL); //левом нижнем углу
	SetViewportOrgEx(hdc, 0, sy, NULL); //
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
	LPARAM lParam)
{
	PAINTSTRUCT ps;
	const int WIDTH = 400;
	const int HEIGHT = 300;
	POINT point1[4];
	HDC hdc;
	int xPos, yPos, zDelta;
	double x[MAX], y[MAX], eps = 10, X, Y, t, xA, xB, xC, xD, yA, yB, yC, yD, a0, a1,
		a2, a3, b0, b1, b2, b3;
	int N = 10, i, j, first;
	static HPEN hDash, hBspline; // два пера
	static HBRUSH hRect, hSel; // две кисти
	static POINT pt[20]; // массив хранения плоских точек
	static POINT point; // структура под одну плоскую точку
	RECT rt; // структура точек прямоугольника
	static int count, index; // счётчик точек,
	static bool capture; // логическая переменная для мыши
	std::ifstream in; //класс файлового потокового вводавывода
	std::ofstream out;
	switch (message)
	{
	case WM_CREATE:
		in.open("dat.txt"); // открытие файлового потока
		if (in.fail())
		{
			MessageBox(hWnd, _T("Файл dat.txt не найден"), _T("Открытие файла"), MB_OK | MB_ICONEXCLAMATION);
			PostQuitMessage(0);
			return 1;
		}
		for (count = 0; in >> pt[count].x; count++) in >> pt[count].y;
		in.close(); // закрытие файлового потока
		hDash = CreatePen(PS_DASH, 1, 0);
		hBspline = CreatePen(PS_SOLID, 4, RGB(255, 70, 0));
		hRect = CreateSolidBrush(RGB(128, 0, 128));
		hSel = CreateSolidBrush(RGB(255, 0, 0));
		break;

	case WM_SIZE:
		sx = LOWORD(lParam); //координата мыши по оси Х
		sy = HIWORD(lParam); //координата мыши по оси У break;
	case WM_LBUTTONDOWN:
		point.x = LOWORD(lParam);
		point.y = HIWORD(lParam);
		//Преобразование экранных координат мыши в логические
		DcInLp(point);
		for (i = 0; i <= count; i++)
		{
			SetRect(&rt, pt[i].x - MARK, pt[i].y - MARK, pt[i].x + MARK, pt[i].y +
				MARK);
			if (PtInRect(&rt, point))
			{
				// Курсор мыши попал в точку
				index = i;
				capture = true;
				hdc = GetDC(hWnd);
				transform(hdc); //Переход в логические координаты
				FillRect(hdc, &rt, hSel);//Отметим прямоугольник цветом
				ReleaseDC(hWnd, hdc);
				SetCapture(hWnd);
				return 0;
			}
		}
		break;
	case WM_LBUTTONUP:
		if (capture)
		{
			ReleaseCapture(); //Освобождение мыши
			capture = false;
		}
		break;
	case WM_MOUSEMOVE:
		if (capture) {
			//Мышь захвачена
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			DcInLp(point); //Преобразование экранных координат мыши
			pt[index] = point; // в логические координаты
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		transform(hdc); //Переход в логические координаты
		SelectObject(hdc, hDash);
		Polyline(hdc, pt, count); //Строим ломанную линию //
		PolyBezier(hdc, pt, count);
		SelectObject(hdc, hBspline);
		for (i = 0; i <= count; i++)
		{
			X = pt[i].x;
			Y = pt[i].y;
			MoveToEx(hdc, X - eps, Y - eps, NULL);
			LineTo(hdc, X + eps, Y + eps);
			MoveToEx(hdc, X + eps, Y - eps, NULL);
			LineTo(hdc, X - eps, Y + eps);
		}
		first = 1;
		for (i = 1; i < count - 1; i++)
		{
			xA = pt[i - 1].x;
			xB = pt[i].x;
			xC = pt[i + 1].x;
			xD = pt[i + 2].x;
			yA = pt[i - 1].y;
			yB = pt[i].y;
			yC = pt[i + 1].y;
			yD = pt[i + 2].y;
			a3 = (-xA + 3 * (xB - xC) + xD) / 6.0;
			b3 = (-yA + 3 * (yB - yC) + yD) / 6.0;
			a2 = (xA - 2 * xB + xC) / 2.0;
			b2 = (yA - 2 * yB + yC) / 2.0;
			a1 = (xC - xA) / 2.0;
			b1 = (yC - yA) / 2.0;
			a0 = (xA + 4 * xB + xC) / 6.0;
			b0 = (yA + 4 * yB + yC) / 6.0;
			for (j = 0; j <= N; j++)
			{
				t = (float)j / (float)N;
				X = ((a3 * t + a2) * t + a1) * t + a0;
				Y = ((b3 * t + b2) * t + b1) * t + b0;
				if (first == 1)
				{
					first = 0;
					MoveToEx(hdc, X, Y, NULL);
				}
				else LineTo(hdc, X, Y);
			}
		}
		for (i = 0; i < count; i++)
		{
			//Закрашиваем точки графика прямоугольниками
			SetRect(&rt, pt[i].x - MARK, pt[i].y - MARK, pt[i].x + MARK, pt[i].y + MARK);
			FillRect(hdc, &rt, hRect);
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		DeleteObject(hDash);
		DeleteObject(hBspline);
		DeleteObject(hRect);
		DeleteObject(hSel);
		PostQuitMessage(0);
		break;
	default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
