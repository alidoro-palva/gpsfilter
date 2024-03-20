#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cctype>
#include <algorithm>

using namespace std;

ifstream fi;      // Входной файл
ofstream fo;      // Выходной файл
string fnamein;   // имя входного файла
string fnameout;  // имя выходного файла
int filelength;   // длина входного файла
int trkptcount;   // количество маршрутных точек
double precision; // Точность

struct tpoint {
    int offsb;
    int offse;
    double lat;
    double lon;
    int mark;
};

vector<tpoint> tpoints;

double pdist(int p1, int p2, double &d2) {
    double qlat = tpoints[p2].lat - tpoints[p1].lat;
    double qlon = tpoints[p2].lon - tpoints[p1].lon;
    d2 = qlat * qlat + qlon * qlon;
    return sqrt(d2);
}

double hpoint(double a, double a2, double b, double b2, double c, double c2) {
    if(a2 >= c2 + b2) return b;
    if(b2 >= c2 + a2) return a;
    double p = (a + b + c) / 2;
    double s = sqrt(p * (p - a) * (p - b) * (p-c));
    return 2 * s / c;
}

void pmax(int p1, int p2, int &m, double &d) {
    double a, b, c, a2, b2, c2;
    m = p1 + 1;
    if(m >= p2) {
        m = 0;
        d = 0;
        return;
    }
    c = pdist(p1, p2, c2);
    d = -1.0;
    for(int i = p1 + 1; i < p2; i++) {
        a = pdist(p1, i, a2);
        b = pdist(p2, i, b2);
        double dd = hpoint(a, a2, b, b2, c, c2);
        if(dd > d) {
            m = i;
            d = dd;
        }
    }
    return;
}

// Фильтрация трека для заданной точности
void filter() {
    for(int i = 0; i < trkptcount; i++)
        tpoints[i].mark = 0; // Убрали все пометки
    tpoints[0].mark = trkptcount - 1; // В начало отрезка ставим указатель на его конец
    tpoints[trkptcount - 1].mark = -1; // В конце ставим не ноль
    // Пока отрезок один
    int pcur = 0; // Текущий отрезок
    while(pcur < trkptcount - 1) { // пока отрезки не кончились
        int m; // индекс наиболее удаленной промежуточной точки
        double d; // удаление этой точки
        int pcur1 = tpoints[pcur].mark; // индекс конечной точки отрезка
        if(pcur1 <= pcur + 1) { // если в отрезке всего две точки,
            pcur = pcur1; // то пропускаем отрезок
        } else {
            pmax(pcur, pcur1, m, d); // находим максимально удаленную точку
            if(d >= precision) { // если удаление не меньше точности,
                tpoints[m].mark = pcur1; // то делим отрезок на два
                tpoints[pcur].mark = m;
                pcur1 = m;
            } else { // иначе переходим к следующему отрезку
                pcur = pcur1;
                pcur1 = tpoints[pcur].mark;
            }
        }
    }
    // Все концы отрезков отмечены, остальные точки можно удалять
    return;
}

int exportfiltered(int r) {
    int b, e, buf, len;
    len = 0;
    b = 0;
    for(int i = 0; i < trkptcount; i++) {
        if(!tpoints[i].mark) {
            e = tpoints[i].offsb;
            if(b < e) {
                len += e - b;
                if(r) {
                    fi.seekg(b, ios::beg);
                    for(int j = b; j < e; j++) {
                        buf = fi.get();
                        fo.put(buf);
                    }
                }
            }
            b = tpoints[i].offse;
        }
    }
    if(r) {
        fi.seekg(b, ios::beg);
        for(;;) {
            buf = fi.get();
            if(fi) fo.put(buf);
            else break;
        }
    }
    return len + filelength - b;
}

int importfile() {
    int reg;
    int buf;
    tpoint bufpoint;
    string coord;
    fi.open(fnamein, ios::in | ios::binary);
    if(!fi) goto err;
    filelength = 0;
    trkptcount = 0;
    reg = 0;
    while(fi) {
        buf = fi.get();
        filelength++;
        switch(reg) {
        // <trkpt
        case 0: // <
            if(buf == '<') {
                reg = 1;
                bufpoint.offsb = filelength - 1;
            }
            break;
        case 1: // t
            if(buf == 't') {
                reg = 2;
            } else {
                reg = 0;
            }
            break;
        case 2: // r
            if(buf == 'r') {
                reg = 3;
            } else {
                reg = 0;
            }
            break;
        case 3: // k
            if(buf == 'k') {
                reg = 4;
            } else {
                reg = 0;
            }
            break;
        case 4: // p
            if(buf == 'p') {
                reg = 5;
            } else {
                reg = 0;
            }
            break;
        case 5: // t
            if(buf == 't') {
                reg = 6;
            } else {
                reg = 0;
            }
            break;
        case 6: // space
            if(isspace(buf)) {
                reg = 7;
                trkptcount++;
                coord = "";
            } else {
                reg = 0;
            }
            break;
        case 7:
            if(buf == '>') {
                size_t pos;
                reg = 8;
                for(int i = coord.length(); --i >= 0;) {
                    if(isspace(coord[i])) coord.erase(i, 1);
                }
                pos=coord.find("lon=\"");
                if(pos != string::npos) {
                    bufpoint.lon = atof(coord.c_str() + pos + 5) * 62500;
                } else {
                    bufpoint.lon = 0.0;
                }
                pos=coord.find("lat=\"");
                if(pos != string::npos) {
                    bufpoint.lat = atof(coord.c_str() + pos + 5) * 111200;
                } else {
                    bufpoint.lat = 0.0;
                }
            } else {
                coord = coord + (char)buf;
            }
            break;
        case 8:
            if(buf == '<') {
                reg = 9;
            }
            break;
        case 9:
            if(buf == '/') {
                reg = 10;
            } else {
                reg = 8;
            }
            break;
        case 10:
            if(buf == 't') {
                reg = 11;
            } else {
                reg = 8;
            }
            break;
        case 11:
            if(buf == 'r') {
                reg = 12;
            } else {
                reg = 8;
            }
            break;
        case 12:
            if(buf == 'k') {
                reg = 13;
            } else {
                reg = 8;
            }
            break;
        case 13:
            if(buf == 'p') {
                reg = 14;
            } else {
                reg = 8;
            }
            break;
        case 14:
            if(buf == 't') {
                reg = 15;
            } else {
                reg = 8;
            }
            break;
        case 15:
            if(isspace(buf)) break;
            if(buf == '>') {
                reg = 16;
            } else {
                reg = 8;
            }
            break;
        case 16:
            if(isspace(buf)) break;
            reg = 0;
            bufpoint.offse = filelength - 1;
            bufpoint.mark = 0;
            tpoints.push_back(bufpoint);
            if(buf == '<') {
                reg = 1;
                bufpoint.offsb = filelength - 1;
            }
            break;
        }
    }
    filelength--;
    fi.close();
    return 0;
err:
    return 1;
}

int main(int npar, char *par[])
{
    cout << "Version 1.0\n";
    if(npar < 2) {
        cout << "Error. Start by\n";
        cout << "gpsfilter.exe <inputfile> [<outputfile>]\n";
        return 1;
    }
    fnamein = par[1];
    if(importfile()== 1) {
        cout << "Unsuccessful import" << endl;
        return 1;
    }
    if(npar > 2) {
        fnameout = par[2];
    } else {
        fnameout = fnamein;
        if(fnamein.length() > 4) {
            string end4 = fnamein.substr(fnamein.length() - 4, 4);
            for(auto it = end4.begin(); it != end4.end(); ++it)
                *it = tolower(*it);
            if(end4 == ".gpx")
                fnameout.insert(fnameout.length() - 4, ".out");
            else
                fnameout += ".out";
        } else {
            fnameout += ".out";
        }
    }
    cout << "Input file " << fnamein.c_str() << endl;
    cout << "Output file " << fnameout << endl;
    cout << "Length of input file\n";
    cout << filelength << endl;
    cout << "Number of trackpoints\n";
    cout << trkptcount << endl;
    if(trkptcount < 3) {
        cout << "To few trackpoints for filtering, hit \"Enter\" to exit.\n";
        cin.get();
        return 0;
    }

    int exitprog = -1;
    while(exitprog) {
        cout << "input 0: exit" << endl;
        cout << "input 1: try filter\n";
        if(exitprog >= 0) {
            cout << "input 2: export rezult and exit\n";
        }
        cin >> exitprog;
        if(exitprog == 1) {
            cout << "Input precision for filtering in meters (for example 3.5)\n";
            cin >> precision;
            filter();
            cout << "Rezults of filtering\n";
            cout << "Length of output file after export\n";
            cout << exportfiltered(0) << endl;
            int rpoints = 0;
            for(int i = 0; i < trkptcount; i++)
                if(tpoints[i].mark) rpoints++;
            cout << "Number of trackpoints\n";
            cout << rpoints << endl;
        }
        if(exitprog == 2) {
            fi.open(fnamein, ios::in | ios::binary);
            fo.open(fnameout, ios::out | ios::binary);
            exportfiltered(1);
            fo.close();
            fi.close();
            exitprog = 0;
        }
    }
    return 0;
}
