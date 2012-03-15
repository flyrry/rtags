#include "RTags.h"

namespace RTags {

int canonicalizePath(char *path, int len)
{
    Q_ASSERT(path[0] == '/');
    for (int i=0; i<len - 3; ++i) {
        if (path[i] == '/' && path[i + 1] == '.'
            && path[i + 2] == '.' && path[i + 3] == '/') {
            for (int j=i - 1; j>=0; --j) {
                if (path[j] == '/') {
                    memmove(path + j, path + i + 3, len - (i + 2));
                    const int removed = (i + 3 - j);
                    len -= removed;
                    i -= removed;
                    break;
                }
            }
        }
    }
    return len;
}

QByteArray unescape(QByteArray command)
{
    command.replace('\'', "\\'");
    command.prepend("bash --norc -c 'echo -n ");
    command.append('\'');
    // QByteArray cmd = "bash --norc -c 'echo -n " + command + "'";
    FILE *f = popen(command.constData(), "r");
    QByteArray ret;
    char buf[1024];
    do {
        const int read = fread(buf, 1, 1024, f);
        if (read)
            ret += QByteArray::fromRawData(buf, read);
    } while (!feof(f));
    fclose(f);
    return ret;
}


QByteArray join(const QList<QByteArray> &list, const QByteArray &sep)
{
    QByteArray ret;
    int size = qMax(0, list.size() - 1) * sep.size();
    foreach(const QByteArray &l, list) {
        size += l.size();
    }
    ret.reserve(size);
    foreach(const QByteArray &l, list) {
        ret.append(l);
    }
    return ret;
}

int readLine(FILE *f, char *buf, int max)
{
    assert(!buf == (max == -1));
    if (max == -1)
        max = INT_MAX;
    for (int i=0; i<max; ++i) {
        const int ch = fgetc(f);
        if (ch == '\n' || ch == EOF) {
            if (buf)
                *buf = '\0';
            return i;
        }
        if (buf)
            *buf++ = *reinterpret_cast<const char*>(&ch);
    }
    return -1;
}

QByteArray context(const Path &path, unsigned offset, unsigned col)
{
    FILE *f = fopen(path.constData(), "r");
    if (f && !fseek(f, offset - (col - 1), SEEK_SET)) {
        char buf[1024] = { '\0' };
        const int len = readLine(f, buf, 1023);
        fclose(f);
        return QByteArray(buf, len);
    }
    return QByteArray();
}

bool makeLocation(const QByteArray &arg, Location *loc,
                  QByteArray *resolvedLocation, const Path &cwd)
{
    Q_ASSERT(!arg.isEmpty());
    int colon = arg.lastIndexOf(':');
    if (colon == arg.size() - 1)
        colon = arg.lastIndexOf(':', colon - 1);
    if (colon == -1) {
        return false;
    }
    const unsigned column = atoi(arg.constData() + colon + 1);
    if (!column) {
        return false;
    }
    const int colon2 = arg.lastIndexOf(':', colon - 1);
    unsigned line = 0;
    if (colon2 != -1) {
        line = atoi(arg.constData() + colon2 + 1);
        if (!line) {
            return false;
        }
        colon = colon2;
    }
    const Path path = Path::resolved(arg.left(colon), cwd);
    if (path.isEmpty()) {
        return false;
    }
    if (resolvedLocation)
        *resolvedLocation = path + arg.mid(colon);
    if (loc) {
        loc->path = path;
        if (line) {
            loc->line = line;
            loc->column = column;
            loc->offset = -1;
        } else {
            loc->offset = column;
        }
    }
    return true;
}

void makeLocation(QByteArray &path, int line, int col)
{
    const int size = path.size();
    const int extra = 2 + digits(line) + digits(col);
    path.resize(size + extra);
    snprintf(path.data() + size, extra + 1, ":%d:%d", line, col);
}


QByteArray makeLocation(const QByteArray &encodedLocation)
{
    Location loc;
    QByteArray file;
    if (makeLocation(encodedLocation, &loc, &file)) {
        const QByteArray ctx = RTags::context(file, loc.line, loc.column);
        return encodedLocation + '\t' + ctx;
    }
    return encodedLocation;
}
}

