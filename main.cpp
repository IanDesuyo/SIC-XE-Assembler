#include <bits/stdc++.h>
using namespace std;

class Assembler {
   public:
    int compile(string);
    void loadInstructions();
    void loadRegisters();
    map<string, pair<int, string>> OPTAB;
    map<string, string> REGS;

   private:
    map<string, int> SYMTAB;
    vector<pair<int, vector<string>>> prog;
    vector<pair<int, string>> obcode;
    vector<int> modify;
    string programName;
    int LOCCTR = 0, base = 0, pc = 0, indx = 0;
    bool checkpc = 0;

    int wordsize(string);
    int bytesize(string);
    int hextoint(string);
    string inttohex(int, int);
    string bintohex(int, int, int, int);
    string readdr(string);
    string format2(int);
    string format3(int);
    string format4(int);
    void writeObjectFile();
    void getInput(string, string &, string &, string &);
};

void Assembler::loadInstructions() {
    ifstream f("instructions.txt");  // opens the instructions file
    string x, y;
    int a;
    while (!f.eof()) {
        // x: instruction
        // a: format
        // y: hex number
        f >> x >> a >> y;
        OPTAB[x] = {a, y};
    }
    f.close();
}

void Assembler::loadRegisters() {
    ifstream f("registers.txt");  // opens the registers file
    string x, y;
    while (!f.eof()) {
        // x: register name
        // y: register code
        f >> x >> y;
        REGS[x] = y;  // maps every register name with it's number
    }
    f.close();
}

int Assembler::compile(string filename) {
    ifstream input(filename);  // opens the input file

    if (input.fail()) {
        cout << "File not found" << endl;
        return 1;
    }

    loadInstructions();
    loadRegisters();

    string l, x, y, z;

    //------------pass 1-----------//
    while (getline(input, l)) {
        getInput(l, x, y, z);
        if (x[0] == '.')
            continue;
        prog.push_back({LOCCTR, {x, y, z}});
        if (x != "*") {
            SYMTAB[x] = LOCCTR;
        }
        if (y == "RESW") {
            LOCCTR += stoi(z) * 3;
        } else if (y == "RESB") {
            LOCCTR += stoi(z);
        } else if (y == "WORD") {
            LOCCTR += wordsize(prog.back().second[2]);
        } else if (y == "BYTE") {
            LOCCTR += bytesize(prog.back().second[2]);
        } else if (y[0] == '+') {
            LOCCTR += 4;
        } else if (OPTAB.find(y) != OPTAB.end()) {
            LOCCTR += OPTAB[y].first;
        }
    }

    //---------------pass 2----------------//
    for (int i = 0; i < prog.size(); i++) {
        string s;
        int j;
        for (j = i; j < prog.size(); j++) {
            if (prog[j].first != prog[i].first)
                break;
        }
        pc = prog[j].first;             // LOCCTR
        string s2 = prog[i].second[1];  // instruction
        int format = OPTAB[prog[i].second[1]].first;
        if (format == 1) {
            s = OPTAB[prog[i].second[1]].second;  // instruction's hex format
        } else if (format == 2) {
            s = format2(i);
        }

        else if (format == 3) {
            s = format3(i);
        } else if (s2[0] == '+') {
            s = format4(i);
            string tt = prog[i].second[2];
            if (tt[0] != '#')
                modify.push_back(prog[i].first);
        }
        if (prog[i].second[1] == "BASE") {
            base = SYMTAB[prog[i].second[2]];
        }
        if (prog[i].second[2] == "LDX") {
            x = SYMTAB[prog[i].second[2]];
        }
        if (prog[i].second[1] == "NOBASE") {
            base = 0;
        } else if (prog[i].second[1] == "BYTE") {
            string adr = prog[i].second[2];
            s = adr.substr(2, adr.size() - 3);
            string s3 = "";
            for (int j = 0; j < s.size(); j++) {
                s3 += inttohex(s[j], 2);
            }
            if (adr[0] == 'C')
                s = s3;
        }

        prog[i].second.push_back(s);
    }

    for (int i = 0; i < prog.size(); i++) {
        if (prog[i].second[0] == "*") {
            if (prog[i].second[2].length() == 8) {
                cout << inttohex(prog[i].first, 4) << "\t\t"
                     << prog[i].second[1] << "\t"
                     << prog[i].second[2] << "  "
                     << prog[i].second[3] << endl;
            } else if (prog[i].second[2] == "*") {
                cout << inttohex(prog[i].first, 4) << "\t\t"
                     << prog[i].second[1] << "\t"
                     << " "
                     << "\t  "
                     << prog[i].second[3] << endl;
            } else {
                cout << inttohex(prog[i].first, 4) << "\t\t"
                     << prog[i].second[1] << "\t"
                     << prog[i].second[2] << "\t  "
                     << prog[i].second[3] << endl;
            }
        } else {
            cout << inttohex(prog[i].first, 4) << "\t"
                 << prog[i].second[0] << "\t"
                 << prog[i].second[1] << "\t"
                 << prog[i].second[2] << "\t  "
                 << prog[i].second[3] << endl;
        }

        if (i == 0)
            programName = prog[i].second[0];
        else if (prog[i].second[3].length() != 0)
            obcode.push_back({prog[i].first, prog[i].second[3]});
    }

    writeObjectFile();
    return 0;
}

int Assembler::wordsize(string s) {
    return s.length() / 2;
}

int Assembler::bytesize(string adr) {
    if (adr[0] == 'C') {
        string s = adr.substr(2, adr.size() - 3);
        return s.size();
    } else if (adr[0] == 'X')
        return 1;
    return 0;
}

int Assembler::hextoint(string hexstring) {
    int number = (int)strtol(hexstring.c_str(), NULL, 16);
    return number;
}

string Assembler::inttohex(int n, int prelength) {
    string s;
    stringstream sstream;
    sstream << setfill('0') << setw(prelength) << hex << (int)n;
    s = sstream.str();
    sstream.clear();
    for (int i = 0; i < s.length(); i++)
        s[i] = toupper(s[i]);
    return s;
}

string Assembler::bintohex(int a, int b, int c, int d) {
    string s;
    int sum = 0;
    sum += (int)d * 1;
    sum += (int)c * 2;
    sum += (int)b * 4;
    sum += (int)a * 8;
    s = inttohex(sum, 1);
    return s;
}

string Assembler::readdr(string res) {
    int x = hextoint(res);
    if (x - pc > -256 && x - pc < 4096) {
        checkpc = 1;
        return inttohex(x - pc, 3);
    } else {
        checkpc = 0;
        return inttohex(x - base, 3);
    }
}

string Assembler::format2(int i) {
    string s, r1, r2 = "A", result;
    s = prog[i].second[2];  // operand
    int j;
    for (j = 0; j < s.size() && s[j] != ','; j++)
        ;
    r1 = s.substr(0, j);
    if (j < s.size())
        r2 = s.substr(j + 1, s.size() - j - 1);
    result = OPTAB[prog[i].second[1]].second;  // instruction's hex format
    result += REGS[r1];
    result += REGS[r2];
    return result;
}

string Assembler::format3(int i) {
    string adr = prog[i].second[2], res1, res2, res3;
    bool nixbpe[6] = {}, dr = 0;
    int no = 0;
    if (adr[adr.size() - 1] == 'X' && adr[adr.size() - 2] == ',') {
        nixbpe[2] = 1;
        adr = adr.substr(0, adr.size() - 2);
    }
    if (adr[0] == '#') {
        nixbpe[1] = 1;
        adr = adr.substr(1, adr.size() - 1);
        if (SYMTAB.find(adr) != SYMTAB.end()) {
            res2 = inttohex(SYMTAB[adr], 3);
        } else {
            res2 = adr;
            dr = 1;
        }
        no = 1;
    } else if (adr[0] == '@') {
        nixbpe[0] = 1;
        adr = adr.substr(1, adr.size() - 1);
        no = 2;
        int z = SYMTAB[adr], j;
        for (j = 0; j < prog.size(); j++)
            if (prog[j].first == z)
                break;
        res2 = adr;
        adr = inttohex(prog[j].first, 3);
        if (prog[j].second[1] != "WORD" && prog[j].second[1] != "BYTE" && prog[j].second[1] != "RESW" && prog[j].second[1] != "RESB") {
            adr = prog[j].second[2];
            z = SYMTAB[adr];
            for (j = 0; j < prog.size(); j++)
                if (prog[j].second[0] == res2)
                    break;
            adr = prog[j].second[2];
            res2 = inttohex(SYMTAB[adr], 3);
        } else {
            res2 = adr;
        }
    } else if (adr[0] == '=') {
        adr = adr.substr(3, adr.size() - 4);
        dr = 1;
    } else {
        res2 = inttohex(SYMTAB[adr], 3);
        nixbpe[0] = 1;
        nixbpe[1] = 1;
        no = 3;
    }
    if (dr != 1 && adr != "*") {
        res2 = readdr(res2);

        res2 = res2.substr(res2.size() - 3, 3);
        nixbpe[4] = checkpc;
        nixbpe[3] = !checkpc;
    }
    if (nixbpe[2] == 1) {
        res2 = inttohex(hextoint(res2) - indx, 3);
    }
    while (res2.size() < 3)
        res2 = "0" + res2;
    res3 = OPTAB[prog[i].second[1]].second;
    res3 = inttohex(hextoint(res3) + no, 2) + bintohex(nixbpe[2], nixbpe[3], nixbpe[4], nixbpe[5]) + res2;
    return res3;
}

string Assembler::format4(int bb) {
    string z = prog[bb].second[2], te = prog[bb].second[1], TA = "", obcode;
    int no = 0;
    bool nixbpe[6] = {0, 0, 0, 0, 0, 0};
    nixbpe[0] = (z[0] == '@');
    nixbpe[1] = (z[0] == '#');
    if (nixbpe[0] == nixbpe[1]) {
        nixbpe[0] = !nixbpe[0];
        nixbpe[1] = !nixbpe[1];
    }
    nixbpe[2] = (z[z.length() - 1] == 'X' && z[z.length() - 2] == ',') ? 1 : 0;
    nixbpe[3] = 0;
    nixbpe[4] = 0;
    nixbpe[5] = 1;
    if (z[0] == '@' || z[0] == '#')
        z = z.substr(1, z.length() - 1);
    if (z[z.length() - 1] == 'X' && z[z.length() - 2] == ',')
        z = z.substr(0, z.length() - 2);
    if (nixbpe[0] == 1 && nixbpe[1] == 1) {
        string s = inttohex(SYMTAB[z], 5);
        for (int i = 0; i < prog.size(); i++) {
            if (inttohex(prog[i].first, 5) == s) {
                if (nixbpe[2] == 0)
                    TA = s;
                else
                    TA = inttohex(hextoint(s) + indx, 5);
            }
        }
        no = 3;
    } else if (nixbpe[0] == 1 && nixbpe[1] == 0 && nixbpe[2] == 0) {
        string s = to_string(SYMTAB[z]);
        for (int i = 0; i < prog.size(); i++)
            if (to_string(prog[i].first) == s) {
                s = prog[i].second[2];
                for (int j = 0; i < prog.size(); j++)
                    if (to_string(prog[j].first) == s)
                        TA = prog[j].second[2];
            }
        no = 2;
    } else if (nixbpe[0] == 0 && nixbpe[1] == 1) {
        if (z[0] < 65)
            TA = inttohex(stoi(z), 5);
        else
            TA = inttohex(SYMTAB[z], 5);
        no = 1;
    }
    string res3 = OPTAB[prog[bb].second[1].substr(1, prog[bb].second[1].size() - 1)].second;

    res3 = inttohex(hextoint(res3) + no, 2) + bintohex(nixbpe[2], nixbpe[3], nixbpe[4], nixbpe[5]) + TA;
    return res3;
}

void Assembler::writeObjectFile() {
    ofstream obcodeFile("output.txt");
    int sz = obcode.size();
    obcodeFile << "H^" << programName;
    for (int j4 = 0; j4 < 6 - programName.size(); j4++)
        obcodeFile << " ";
    obcodeFile << "^" << inttohex(obcode[0].first, 6) << "^" << inttohex(LOCCTR, 6) << endl;

    for (int i = 0; i < obcode.size(); i += 5) {
        long long sum = 0;
        for (int j = i; j < i + min(sz - i, 5); j++) {
            sum += obcode[j].second.size() / 2;
        }
        obcodeFile << "T^" << inttohex(obcode[i].first, 6) << "^" << inttohex(sum, 2);

        for (int j = i; j < i + min(sz - i, 5); j++) {
            obcodeFile << "^" << obcode[j].second;
        }
        obcodeFile << endl;
    }
    for (int i = 0; i < modify.size(); i++)
        obcodeFile << "M^" << inttohex(modify[i] + 1, 6) << "^05" << endl;
    obcodeFile << "E^" << inttohex(obcode[0].first, 6);
    obcodeFile.close();
}

void Assembler::getInput(string l, string &a, string &b, string &c) {
    string x, y, z;

    if (l[0] == ' ')
        x = "*";
    else {
        int j = 0;
        for (; j < l.size() && l[j] != ' '; j++)
            ;
        x = l.substr(0, j);
    }
    l = l.substr(x.size() + 1, l.size() - x.size() - 1);
    int j = 0;
    for (; j < l.size() && l[j] != ' '; j++)
        ;
    y = l.substr(0, j);
    l = l.substr(y.size() + 1, l.size() - y.size() - 1);
    if (l[0] == ' ')
        z = "*";
    else {
        z = l;
    }

    a = x;
    b = y;
    c = z;
}

int main() {
    Assembler a;
    string filename;

    do {
        cout << "Enter the file name: ";
        cin >> filename;

    } while (a.compile(filename) != 0);

    return 0;
}