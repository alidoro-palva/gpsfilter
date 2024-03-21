""" Преобразование маршрутных точек формата plt
    (каждая строка это одна точка) в формат gpx (элемент trkp).
    Первый параметр запуска это имя исходного файла.
    Имя файла с результатом обработки формируется добавлением
    ".gpx" к исходному имени.
"""
import datetime as dt
import sys
filename = sys.argv[1]
fi = open(filename, "rt", encoding="cp1251")
fo = open(filename + ".gpx", "wt")
while True:
    s = fi.readline()
    if not s:
        break
    p = s.split(",")
    for i in range(len(p)):
        p[i] = p[i].strip()
    fo.write(f'      <trkpt lat="{p[0]}" lon="{p[1]}">\n')
    fo.write(f'        <ele>{p[3]}</ele>\n')
    d, t = p[4].split(".")
    # d == "1" for 1899-12-31
    d = int(d) + 693594  Число дней
    # теперь d == 1 соответствует 0001-01-01
    d = dt.date.fromordinal(d)
    t = round(float("." + t) * 24 * 60 * 60)
    t, s = divmod(t, 60)
    h, m = divmod(t, 60)
    t = dt.time(h, m, s)
    fo.write(f'        <time>{d}T{t}Z</time>\n')
    fo.write("      </trkpt>\n")
fo.close()
fi.close()