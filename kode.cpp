#include <iostream>
#include <fstream>
#include <locale>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>

using namespace std;

// definiujemy sta�e
constexpr int columns = 20;
constexpr int rows = 20;

constexpr const char* fileName = "grid.txt";

// indeksy zaczynaj� si� od 0
// 0,0 uk�adu wsp�rz�dnych jest w lewym dolnym rogu
// x - o� pozioma (kolumny)
// y - o� pionowa (wiersze)
constexpr int targetX = 19;
constexpr int targetY = 19;
constexpr int startX = 0;
constexpr int startY = 0;

// pomocnicze sta�e do rysowania mapy w konsoli
constexpr const char* wallSymbol = "\xDB\xDB";
constexpr const char* hlineSymbol = "\xC4\xC4";
constexpr const char* vlineSymbol = "\xB3";
//constexpr const char* pathSymbol = "\xB0\xB0";
constexpr const char* pathSymbol = "#3";
constexpr const char* emptySpaceSymbol = "  ";

// pomocnicza flaga okre�laj�ca od razu pokaza� wynik czy pokazywa� kolejne kroki dzia�ania algorytmu
// false - pokazuje kolejne kroki algorytmu, przej�cie mi�dzy krokami nast�puje enterem
// true - od razu wskazuje wynik
constexpr bool fastResultMode = true;

// zmienna pomocnicza je�li chcesz wygenerowa� w�asn� map�
// true - generuje map�
// false - wczytuje map� z pliku
constexpr bool generateOwnMap = false;

// definujemy pomocniczne typy wyliczeniowe
enum makeStepResult
{
    targetFound,
    canContinue,
    emptyList
};

enum gridPointState : int
{
    empty = 0,
    path = 3,
    wall = 5
};

// definujemy struktur� "Point"
struct Point
{
    // konstruktor struktury Point
    Point(int x, int y, Point* parent) : x(x), y(y), parent(parent) {}

    // wsp�rz�dne na mapie
    int x;
    int y;

    // zmienne do A*
    double f = 0;
    int g = 0;

    // rodzic
    Point* parent = nullptr;
};

// funkcja odczytuj�ca map� z pliku
bool readGridFromFile(int** grid, string fname)
{
    ifstream file(fname.c_str());

    if (!file)
    {
        cout << "Blad otwarcia pliku\n";
        return false;
    }

    // zak�adam, �e struktura pliku jest zgodna z za�o�eniami
    // punkty maj� wartosci 5 lub 0
    // jest odpowiednia liczba punkt�w (rows x columns)
    // punkt startowy i docelowy nie s� murem (warto�� 5)
    for (auto j = rows - 1; j >= 0; j--)
    {
        for (auto i = 0; i < columns; i++)
        {
            file >> grid[i][j];
        }
    }

    file.close();

    return true;
}

// funkcja rysuj�ca map� w konsoli
void drawGrid(int** grid)
{
    cout << "\n ";
    for (auto i = 0; i < columns; i++) 
        cout << hlineSymbol;
    cout << "\n";

    for (auto j = rows - 1; j >= 0; j--)
    {
        cout << vlineSymbol;
        for (auto i = 0; i < columns; i++)
        {
            if (grid[i][j] == gridPointState::wall)
                cout << wallSymbol;
            else if (grid[i][j] == gridPointState::path)
                cout << pathSymbol;
            else
                cout << emptySpaceSymbol;
        }
        cout << vlineSymbol << "\n";
    }

    cout << " ";
    for (auto i = 0; i < columns; i++)
        cout << hlineSymbol;
    cout << "\n";
}

// funkcja zaznaczaj�ca �cie�k�
void markPath(int** grid, std::vector<Point*>& visitedPointsList)
{
    for (auto j = rows - 1; j >= 0; j--)
    {
        for (auto i = 0; i < columns; i++)
        {
            if (grid[i][j] == gridPointState::path)
                grid[i][j] = gridPointState::empty;
        }
    }

    auto point = visitedPointsList.back();
    while (point->x != startX || point->y != startY)
    {
        grid[point->x][point->y] = gridPointState::path;
        point = point->parent;
    }
    grid[point->x][point->y] = gridPointState::path;
}

// najwa�niejsza funkcja programu, zawieraj�ca ca�� logik� algorytmu A*
makeStepResult makeStep(int** grid, std::vector<Point*>& visitedPointsList, std::vector<Point*>& possibleNextStepsList)
{
    // tworzy pomocniczą listę kandydat�w na nast�pny krok
    std::vector<Point> stepCandidatesList;

    // pobieramy ostatnio odwiedzone pole
    Point* last = visitedPointsList.back();

    // jeśli ostatnio odwiedzone pole jest celem to algorytm kończy działanie
    if (last->x == targetX && last->y == targetY)
        return makeStepResult::targetFound;

    // ruch w górę
    if (last->y < rows - 1 && grid[last->x][last->y + 1] == gridPointState::empty)
        stepCandidatesList.push_back(Point(last->x, last->y + 1, last));

    // ruch w dół
    if (last->y > 0 && grid[last->x][last->y - 1] == gridPointState::empty)
        stepCandidatesList.push_back(Point(last->x, last->y - 1, last));

    // ruch w lewo
    if (last->x > 0 && grid[last->x - 1][last->y] == gridPointState::empty)
        stepCandidatesList.push_back(Point(last->x - 1, last->y, last));

    // ruch w prawo
    if (last->x < columns - 1 && grid[last->x + 1][last->y] == gridPointState::empty)
        stepCandidatesList.push_back(Point(last->x + 1, last->y, last));

    // dla ka�dego kandydata do listy otwartej wykonuj� poni�szy kod 
    for (auto& step : stepCandidatesList)
    {
        // obliczam heurystyk� h(x)
        auto h = sqrt(pow((step.x - targetX), 2) + pow((step.y - targetY), 2));

        // obliczam koszt dotarcia f(x) = g(x) + h(x)
        step.g = last->g + 1;
        step.f = step.g + h;

        // sprawdzam czy ruch znajduje si� ju� na li�cie otwartej
        Point* founded = nullptr;
        
        for (auto it = possibleNextStepsList.begin(); it < possibleNextStepsList.end(); it++)
        {
            if ((*it)->x == step.x && (*it)->y == step.y)
            {
                founded = *it;
                break;
            }
        }
   
        if (founded) // je�li znajduje si� na li�cie otwartej to
        {
            if (step.f < founded->f) // sprawdzam czy nale�y zaktualizowa� koszt dotarcia i rodzica
            {
                founded->f = step.f;
                founded->parent = last;
            }
        }
        else // je�li ruchu nie by�o na li�cie otwartej to go dodaj� do listy
            possibleNextStepsList.push_back(new Point(step));
    }

    stepCandidatesList.clear();

    // je�li lista otwarta jest pusta algorytm A* ko�czy dzia�anie, poniewa� nie mo�na doj�� do celu 
    if (possibleNextStepsList.empty())
        return makeStepResult::emptyList;

    // z listy otwartej wybieram najbli�szy ruch (o najmniejszym koszcie f(x))
    // konflikty rozwi�wyane s� hierarchicznie, wybieramy spo�r�d kratek o tym samym f(x) pole odwiedzone najp�niej
    Point* nearestStep = possibleNextStepsList.front();
    for (auto it = possibleNextStepsList.begin() + 1; it != possibleNextStepsList.end(); it++)
    {
        if ((*it)->f <= nearestStep->f)
            nearestStep = *it;
    }

    // przenosz� ruch z listy otwartej do zamkni�tej
    visitedPointsList.push_back(nearestStep);
    possibleNextStepsList.erase(std::remove(possibleNextStepsList.begin(), possibleNextStepsList.end(), nearestStep), possibleNextStepsList.end());

    // zaznaczam odpowiedni� kratk� jako odwiedzon�
    grid[nearestStep->x][nearestStep->y] = gridPointState::path;

    return makeStepResult::canContinue;
}

// funkcja generuj�ca map�
// randValue < 0.8 oznacza, �e 80% przestrzeni b�dzie puste, a 20% b�dzie murem
void generateMap(int** grid) {
    std::srand(static_cast<unsigned int>(std::time(0)));

    for (int i = 0; i < columns; ++i) 
    {
        for (int j = 0; j < rows; ++j) 
        {
            auto randValue = static_cast<double>(std::rand()) / RAND_MAX;
            grid[i][j] = (randValue < 0.8) ? gridPointState::empty : gridPointState::wall;
        }
    }

    grid[startX][startY] = gridPointState::empty;
    grid[targetX][targetY] = gridPointState::empty;
}

int main(void)
{
    //deklarujemy dynamiczn� tablic�, do kt�rej wczytamy nasz plik
    int** grid = new int* [columns];
    for (auto i = 0; i < columns; i++)
        grid[i] = new int[rows];

    // wczytujemy map� z pliku lub generujemy w�asn� map�
    if (generateOwnMap)
        generateMap(grid);
    else
    {
        if (readGridFromFile(grid, fileName))
            cout << "Wczytano mape z pliku\n\n";
        else
        {
            cout << "Blad odczytania mapy z pliku\n\n";
            return 0;
        }
    }

    grid[startX][startY] = gridPointState::path;

    // tworzymy list� otwart�, zawieraj�c� punkty rozwa�ane jako pola do ekspansji
    std::vector<Point*> possibleNextStepsList;
    // tworzymy list� zamkni�t�, zawieraj�c� odwiedzone punkty
    Point* startPoint = new Point(startX, startY, nullptr);
    std::vector<Point*> visitedPointsList = { startPoint };

    // wykonujemy kolejne kroki algorytmu A*
    makeStepResult result = makeStepResult::canContinue;
    while (result == makeStepResult::canContinue)
    {
        if (!fastResultMode)
        {
            drawGrid(grid);

            cout << "\n\nNacisnij ENTER, aby wykonac nastepny krok";
            while (getchar() != '\n');
        }

        result = makeStep(grid, visitedPointsList, possibleNextStepsList);
    }

    // prezentujemy wynik dzia�ania u�ytkownikowi
    if (result == makeStepResult::emptyList)
        cout << "\n\nNie mozna znalesc sciezki";
    else
    {
        cout << "\n\nZnaleziono sciezke";
        markPath(grid, visitedPointsList);
        drawGrid(grid);
    }

    //na koniec czy�cimy pami��
    for (auto i = 0; i < columns; i++)
        delete[] grid[i];
    delete[] grid;

    for (auto& it : possibleNextStepsList)
        delete it;

    for (auto& it : visitedPointsList)
        delete it;

    cout << "\n\nNacisnij ENTER aby zakonczyc";
    while (getchar() != '\n');

    return 0;
}
