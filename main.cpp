#include "raylib.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;

const int SCREEN_W = 1000;
const int SCREEN_H = 900;
const float CARD_W = 100;
const float CARD_H = 145;
const float GAP_X = 40;
const float MARGIN_X = 30;
const float TOP_Y = 20;
const float TABLEAU_Y = 200;
const float DOWN_OFFSET = 18;
const float UP_OFFSET = 32;
const float BAR_Y = 860;

enum Suit { HEARTS, DIAMONDS, CLUBS, SPADES };
enum PileType { NO_PILE, STOCK_PILE, WASTE_PILE, FOUNDATION_PILE, TABLEAU_PILE };
enum MoveKind { CARD_MOVE, DRAW_MOVE, RECYCLE_MOVE };

Color feltGreen = {26, 94, 60, 255};
Color barColor = {15, 50, 35, 255};
Color buttonColor = {45, 95, 70, 255};
Color buttonHover = {70, 130, 100, 255};
Color backBlue = {38, 70, 150, 255};
Color backLight = {120, 150, 220, 255};
Color hintGold = {255, 215, 0, 255};
Color slotShade = {0, 0, 0, 40};
Color slotLine = {255, 255, 255, 70};
Color slotText = {255, 255, 255, 90};
Color cardEdge = {0, 0, 0, 80};
Color dimmer = {0, 0, 0, 160};

Rectangle buttons[5] = {
    {20, 865, 80, 30},
    {110, 865, 80, 30},
    {200, 865, 80, 30},
    {290, 865, 110, 30},
    {410, 865, 90, 30}
};

class Card {
public:
    int suit;
    int rank;
    bool faceUp;

    Card() {
        suit = 0;
        rank = 1;
        faceUp = false;
    }

    Card(int s, int r) {
        suit = s;
        rank = r;
        faceUp = false;
    }

    bool isRed() const {
        return suit == HEARTS || suit == DIAMONDS;
    }

    int id() const {
        return suit * 13 + rank - 1;
    }
};

struct Location {
    int type;
    int index;
};

struct Move {
    int kind;
    int fromType;
    int fromIdx;
    int toType;
    int toIdx;
    int count;
    int drawn;
    bool flipped;
    int scoreChange;
};

template <class T>
class Stack {
private:
    struct Node {
        T data;
        Node* next;
    };
    Node* topNode;
    int count;
    Stack(const Stack&);
    Stack& operator=(const Stack&);

public:
    Stack() {
        topNode = NULL;
        count = 0;
    }

    ~Stack() {
        clear();
    }

    void push(T value) {
        Node* n = new Node;
        n->data = value;
        n->next = topNode;
        topNode = n;
        count++;
    }

    T pop() {
        Node* n = topNode;
        T value = n->data;
        topNode = n->next;
        delete n;
        count--;
        return value;
    }

    T top() const {
        return topNode->data;
    }

    bool isEmpty() const {
        return count == 0;
    }

    int size() const {
        return count;
    }

    void clear() {
        while (topNode != NULL) {
            Node* n = topNode;
            topNode = topNode->next;
            delete n;
        }
        count = 0;
    }
};

template <class T>
class Queue {
private:
    struct Node {
        T data;
        Node* next;
    };
    Node* frontNode;
    Node* backNode;
    int count;
    Queue(const Queue&);
    Queue& operator=(const Queue&);

public:
    Queue() {
        frontNode = NULL;
        backNode = NULL;
        count = 0;
    }

    ~Queue() {
        clear();
    }

    void enqueue(T value) {
        Node* n = new Node;
        n->data = value;
        n->next = NULL;
        if (backNode != NULL) {
            backNode->next = n;
        } else {
            frontNode = n;
        }
        backNode = n;
        count++;
    }

    void enqueueFront(T value) {
        Node* n = new Node;
        n->data = value;
        n->next = frontNode;
        frontNode = n;
        if (backNode == NULL) {
            backNode = n;
        }
        count++;
    }

    T dequeue() {
        Node* n = frontNode;
        T value = n->data;
        frontNode = n->next;
        if (frontNode == NULL) {
            backNode = NULL;
        }
        delete n;
        count--;
        return value;
    }

    bool isEmpty() const {
        return count == 0;
    }

    int size() const {
        return count;
    }

    void clear() {
        while (frontNode != NULL) {
            Node* n = frontNode;
            frontNode = frontNode->next;
            delete n;
        }
        backNode = NULL;
        count = 0;
    }
};

class Pile {
private:
    struct Node {
        Card* card;
        Node* next;
    };
    Node* head;
    Node* tail;
    int count;
    Pile(const Pile&);
    Pile& operator=(const Pile&);

public:
    Pile() {
        head = NULL;
        tail = NULL;
        count = 0;
    }

    ~Pile() {
        clear();
    }

    void addTop(Card* c) {
        Node* n = new Node;
        n->card = c;
        n->next = NULL;
        if (tail != NULL) {
            tail->next = n;
        } else {
            head = n;
        }
        tail = n;
        count++;
    }

    Card* removeTop() {
        Node* cur = head;
        Node* prev = NULL;
        while (cur->next != NULL) {
            prev = cur;
            cur = cur->next;
        }
        Card* c = cur->card;
        if (prev != NULL) {
            prev->next = NULL;
            tail = prev;
        } else {
            head = NULL;
            tail = NULL;
        }
        delete cur;
        count--;
        return c;
    }

    Card* top() const {
        if (tail == NULL) {
            return NULL;
        }
        return tail->card;
    }

    Card* at(int index) const {
        Node* cur = head;
        for (int i = 0; i < index; i++) {
            cur = cur->next;
        }
        return cur->card;
    }

    int size() const {
        return count;
    }

    void clear() {
        while (head != NULL) {
            Node* n = head;
            head = head->next;
            delete n;
        }
        tail = NULL;
        count = 0;
    }
};

float colX(int col) {
    return MARGIN_X + col * (CARD_W + GAP_X);
}

string cardFileName(int suit, int rank) {
    const char* suitNames[4] = {"hearts", "diamonds", "clubs", "spades"};
    string rankName;
    if (rank == 1) {
        rankName = "ace";
    } else if (rank == 11) {
        rankName = "jack";
    } else if (rank == 12) {
        rankName = "queen";
    } else if (rank == 13) {
        rankName = "king";
    } else {
        rankName = to_string(rank);
    }
    string name = "card_images/" + rankName + "_of_" + suitNames[suit];
    if (rank >= 11 || (rank == 1 && suit == SPADES)) {
        name += "2";
    }
    return name + ".png";
}

Move makeMove(int kind, int fromType, int fromIdx, int toType, int toIdx, int count) {
    Move m;
    m.kind = kind;
    m.fromType = fromType;
    m.fromIdx = fromIdx;
    m.toType = toType;
    m.toIdx = toIdx;
    m.count = count;
    m.drawn = 0;
    m.flipped = false;
    m.scoreChange = 0;
    return m;
}

class Game {
public:
    Card cards[52];
    vector<Card*> deck;
    Queue<Card*> stock;
    Pile waste;
    Stack<Card*> foundation[4];
    Pile tableau[7];
    unordered_map<int, Location> where;
    Stack<Move> undoStack;
    Stack<Move> redoStack;
    Texture2D faces[52];

    int drawCount;
    int score;
    int moves;
    float elapsed;
    bool won;

    bool dragging;
    int dragType;
    int dragIdx;
    int dragCount;
    float dragOffX;
    float dragOffY;
    vector<Card*> dragCards;
    double lastClickTime;
    int lastClickId;

    int hintMode;
    int hintSrcType;
    int hintSrcIdx;
    int hintSrcPos;
    int hintDstType;
    int hintDstIdx;
    float hintTimer;

    Game() {
        for (int s = 0; s < 4; s++) {
            for (int r = 1; r <= 13; r++) {
                Image img = LoadImage(cardFileName(s, r).c_str());
                ImageResize(&img, (int)CARD_W, (int)CARD_H);
                faces[s * 13 + r - 1] = LoadTextureFromImage(img);
                UnloadImage(img);
            }
        }
        drawCount = 1;
        lastClickTime = 0;
        lastClickId = -1;
        newGame();
    }

    ~Game() {
        for (int i = 0; i < 52; i++) {
            UnloadTexture(faces[i]);
        }
    }

    void setWhere(Card* c, int type, int index) {
        Location loc;
        loc.type = type;
        loc.index = index;
        where[c->id()] = loc;
    }

    void shuffleDeck() {
        for (int i = 51; i > 0; i--) {
            int j = rand() % (i + 1);
            swap(deck[i], deck[j]);
        }
    }

    void newGame() {
        stock.clear();
        waste.clear();
        for (int i = 0; i < 4; i++) {
            foundation[i].clear();
        }
        for (int i = 0; i < 7; i++) {
            tableau[i].clear();
        }
        undoStack.clear();
        redoStack.clear();
        where.clear();
        dragCards.clear();
        deck.clear();

        for (int i = 0; i < 52; i++) {
            cards[i] = Card(i / 13, i % 13 + 1);
            deck.push_back(&cards[i]);
        }
        shuffleDeck();

        int next = 0;
        for (int col = 0; col < 7; col++) {
            for (int row = 0; row <= col; row++) {
                Card* c = deck[next++];
                c->faceUp = (row == col);
                tableau[col].addTop(c);
                setWhere(c, TABLEAU_PILE, col);
            }
        }
        while (next < 52) {
            Card* c = deck[next++];
            c->faceUp = false;
            stock.enqueue(c);
            setWhere(c, STOCK_PILE, 0);
        }

        score = 0;
        moves = 0;
        elapsed = 0;
        won = false;
        dragging = false;
        hintMode = 0;
        hintTimer = 0;
    }

    bool canGoToFoundation(Card* c, int pile) {
        if (c->suit != pile) {
            return false;
        }
        if (foundation[pile].isEmpty()) {
            return c->rank == 1;
        }
        return foundation[pile].top()->rank == c->rank - 1;
    }

    bool canGoToTableau(Card* c, int col) {
        Card* t = tableau[col].top();
        if (t == NULL) {
            return c->rank == 13;
        }
        return t->faceUp && t->isRed() != c->isRed() && t->rank == c->rank + 1;
    }

    vector<Card*> takeCards(int type, int idx, int count) {
        vector<Card*> out;
        if (type == TABLEAU_PILE) {
            for (int i = 0; i < count; i++) {
                out.push_back(tableau[idx].removeTop());
            }
        } else if (type == WASTE_PILE) {
            out.push_back(waste.removeTop());
        } else if (type == FOUNDATION_PILE) {
            out.push_back(foundation[idx].pop());
        }
        reverse(out.begin(), out.end());
        return out;
    }

    void putCards(int type, int idx, vector<Card*>& list) {
        for (int i = 0; i < (int)list.size(); i++) {
            Card* c = list[i];
            if (type == TABLEAU_PILE) {
                tableau[idx].addTop(c);
            } else if (type == WASTE_PILE) {
                waste.addTop(c);
            } else if (type == FOUNDATION_PILE) {
                foundation[idx].push(c);
            }
            setWhere(c, type, idx);
        }
    }

    void applyCardMove(Move& m) {
        vector<Card*> moving = takeCards(m.fromType, m.fromIdx, m.count);
        putCards(m.toType, m.toIdx, moving);
        m.flipped = false;
        int points = 0;
        if (m.toType == FOUNDATION_PILE && m.fromType != FOUNDATION_PILE) {
            points += 10;
        }
        if (m.toType == TABLEAU_PILE && m.fromType == WASTE_PILE) {
            points += 5;
        }
        if (m.toType == TABLEAU_PILE && m.fromType == FOUNDATION_PILE) {
            points -= 15;
        }
        if (m.fromType == TABLEAU_PILE) {
            Card* t = tableau[m.fromIdx].top();
            if (t != NULL && !t->faceUp) {
                t->faceUp = true;
                m.flipped = true;
                points += 5;
            }
        }
        m.scoreChange = points;
    }

    void applyDraw(Move& m) {
        int n = 0;
        for (int i = 0; i < drawCount && !stock.isEmpty(); i++) {
            Card* c = stock.dequeue();
            c->faceUp = true;
            waste.addTop(c);
            setWhere(c, WASTE_PILE, 0);
            n++;
        }
        m.drawn = n;
        m.scoreChange = 0;
    }

    void applyRecycle(Move& m) {
        vector<Card*> temp;
        while (waste.size() > 0) {
            temp.push_back(waste.removeTop());
        }
        for (int i = (int)temp.size() - 1; i >= 0; i--) {
            temp[i]->faceUp = false;
            stock.enqueue(temp[i]);
            setWhere(temp[i], STOCK_PILE, 0);
        }
        m.scoreChange = -50;
    }

    void applyMove(Move& m) {
        if (m.kind == DRAW_MOVE) {
            applyDraw(m);
        } else if (m.kind == RECYCLE_MOVE) {
            applyRecycle(m);
        } else {
            applyCardMove(m);
        }
        score += m.scoreChange;
    }

    void revertMove(Move& m) {
        if (m.kind == DRAW_MOVE) {
            for (int i = 0; i < m.drawn; i++) {
                Card* c = waste.removeTop();
                c->faceUp = false;
                stock.enqueueFront(c);
                setWhere(c, STOCK_PILE, 0);
            }
        } else if (m.kind == RECYCLE_MOVE) {
            while (!stock.isEmpty()) {
                Card* c = stock.dequeue();
                c->faceUp = true;
                waste.addTop(c);
                setWhere(c, WASTE_PILE, 0);
            }
        } else {
            if (m.flipped) {
                tableau[m.fromIdx].top()->faceUp = false;
            }
            vector<Card*> moving = takeCards(m.toType, m.toIdx, m.count);
            putCards(m.fromType, m.fromIdx, moving);
        }
        score -= m.scoreChange;
    }

    void checkWin() {
        int total = 0;
        for (int i = 0; i < 4; i++) {
            total += foundation[i].size();
        }
        won = (total == 52);
    }

    void performMove(Move m) {
        applyMove(m);
        undoStack.push(m);
        redoStack.clear();
        moves++;
        hintTimer = 0;
        checkWin();
    }

    void undo() {
        if (undoStack.isEmpty()) {
            return;
        }
        Move m = undoStack.pop();
        revertMove(m);
        redoStack.push(m);
        if (moves > 0) {
            moves--;
        }
        won = false;
        hintTimer = 0;
    }

    void redo() {
        if (redoStack.isEmpty()) {
            return;
        }
        Move m = redoStack.pop();
        applyMove(m);
        undoStack.push(m);
        moves++;
        hintTimer = 0;
        checkWin();
    }

    void drawFromStock() {
        if (!stock.isEmpty()) {
            performMove(makeMove(DRAW_MOVE, STOCK_PILE, 0, WASTE_PILE, 0, 0));
        } else if (waste.size() > 0) {
            performMove(makeMove(RECYCLE_MOVE, WASTE_PILE, 0, STOCK_PILE, 0, 0));
        }
    }

    void toggleDrawMode() {
        drawCount = (drawCount == 1) ? 3 : 1;
        newGame();
    }

    int wasteShown() {
        int n = drawCount;
        if (waste.size() < n) {
            n = waste.size();
        }
        return n;
    }

    float wasteX(int slot) {
        return colX(1) + slot * 28;
    }

    float tableauCardY(int col, int pos) {
        float y = TABLEAU_Y;
        for (int k = 0; k < pos; k++) {
            if (tableau[col].at(k)->faceUp) {
                y += UP_OFFSET;
            } else {
                y += DOWN_OFFSET;
            }
        }
        return y;
    }

    void setHint(int mode, int srcType, int srcIdx, int srcPos, int dstType, int dstIdx) {
        hintMode = mode;
        hintSrcType = srcType;
        hintSrcIdx = srcIdx;
        hintSrcPos = srcPos;
        hintDstType = dstType;
        hintDstIdx = dstIdx;
        hintTimer = 3;
    }

    void findHint() {
        hintMode = 0;
        hintTimer = 0;
        Card* w = waste.top();

        if (w != NULL && canGoToFoundation(w, w->suit)) {
            setHint(1, WASTE_PILE, 0, waste.size() - 1, FOUNDATION_PILE, w->suit);
            return;
        }
        for (int col = 0; col < 7; col++) {
            Card* t = tableau[col].top();
            if (t != NULL && t->faceUp && canGoToFoundation(t, t->suit)) {
                setHint(1, TABLEAU_PILE, col, tableau[col].size() - 1, FOUNDATION_PILE, t->suit);
                return;
            }
        }
        for (int col = 0; col < 7; col++) {
            for (int pos = 1; pos < tableau[col].size(); pos++) {
                Card* c = tableau[col].at(pos);
                bool reveals = !tableau[col].at(pos - 1)->faceUp;
                if (!c->faceUp || !reveals) {
                    continue;
                }
                for (int dst = 0; dst < 7; dst++) {
                    if (dst != col && canGoToTableau(c, dst)) {
                        setHint(1, TABLEAU_PILE, col, pos, TABLEAU_PILE, dst);
                        return;
                    }
                }
            }
        }
        if (w != NULL) {
            for (int dst = 0; dst < 7; dst++) {
                if (canGoToTableau(w, dst)) {
                    setHint(1, WASTE_PILE, 0, waste.size() - 1, TABLEAU_PILE, dst);
                    return;
                }
            }
        }
        if (!stock.isEmpty() || waste.size() > 0) {
            setHint(2, STOCK_PILE, 0, 0, STOCK_PILE, 0);
        }
    }

    void startDrag(int type, int idx, int pos, Vector2 mouse) {
        dragCards.clear();
        if (type == TABLEAU_PILE) {
            for (int k = pos; k < tableau[idx].size(); k++) {
                dragCards.push_back(tableau[idx].at(k));
            }
        } else if (type == WASTE_PILE) {
            dragCards.push_back(waste.top());
        } else {
            dragCards.push_back(foundation[idx].top());
        }

        Card* first = dragCards[0];
        Location loc = where[first->id()];

        if (GetTime() - lastClickTime < 0.35 && lastClickId == first->id() && dragCards.size() == 1 && loc.type != FOUNDATION_PILE) {
            lastClickTime = 0;
            if (canGoToFoundation(first, first->suit)) {
                performMove(makeMove(CARD_MOVE, loc.type, loc.index, FOUNDATION_PILE, first->suit, 1));
                dragCards.clear();
                return;
            }
        }
        lastClickTime = GetTime();
        lastClickId = first->id();

        float cx = 0;
        float cy = TOP_Y;
        if (loc.type == TABLEAU_PILE) {
            cx = colX(loc.index);
            cy = tableauCardY(loc.index, pos);
        } else if (loc.type == WASTE_PILE) {
            cx = wasteX(wasteShown() - 1);
        } else {
            cx = colX(3 + loc.index);
        }

        dragging = true;
        dragType = loc.type;
        dragIdx = loc.index;
        dragCount = (int)dragCards.size();
        dragOffX = mouse.x - cx;
        dragOffY = mouse.y - cy;
    }

    void handlePress() {
        Vector2 m = GetMousePosition();

        for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(m, buttons[i])) {
                if (i == 0) undo();
                else if (i == 1) redo();
                else if (i == 2) findHint();
                else if (i == 3) newGame();
                else toggleDrawMode();
                return;
            }
        }
        if (won) {
            return;
        }

        Rectangle stockRect = {colX(0), TOP_Y, CARD_W, CARD_H};
        if (CheckCollisionPointRec(m, stockRect)) {
            drawFromStock();
            return;
        }

        int shown = wasteShown();
        if (shown > 0) {
            Rectangle r = {wasteX(shown - 1), TOP_Y, CARD_W, CARD_H};
            if (CheckCollisionPointRec(m, r)) {
                startDrag(WASTE_PILE, 0, waste.size() - 1, m);
                return;
            }
        }

        for (int i = 0; i < 4; i++) {
            Rectangle r = {colX(3 + i), TOP_Y, CARD_W, CARD_H};
            if (CheckCollisionPointRec(m, r) && !foundation[i].isEmpty()) {
                startDrag(FOUNDATION_PILE, i, 0, m);
                return;
            }
        }

        for (int col = 0; col < 7; col++) {
            int n = tableau[col].size();
            for (int pos = n - 1; pos >= 0; pos--) {
                Card* c = tableau[col].at(pos);
                float h = CARD_H;
                if (pos != n - 1) {
                    h = c->faceUp ? UP_OFFSET : DOWN_OFFSET;
                }
                Rectangle r = {colX(col), tableauCardY(col, pos), CARD_W, h};
                if (CheckCollisionPointRec(m, r)) {
                    if (c->faceUp) {
                        startDrag(TABLEAU_PILE, col, pos, m);
                    }
                    return;
                }
            }
        }
    }

    void handleRelease() {
        if (!dragging) {
            return;
        }
        dragging = false;
        Vector2 m = GetMousePosition();
        Card* first = dragCards[0];
        float cx = m.x - dragOffX + CARD_W / 2;
        float cy = m.y - dragOffY + CARD_H / 2;

        if (cy < 175) {
            bool inZone = cx >= colX(3) - 20 && cx <= colX(6) + CARD_W + 20;
            if (inZone && dragCount == 1 && !(dragType == FOUNDATION_PILE && dragIdx == first->suit)) {
                if (canGoToFoundation(first, first->suit)) {
                    performMove(makeMove(CARD_MOVE, dragType, dragIdx, FOUNDATION_PILE, first->suit, 1));
                }
            }
        } else {
            for (int col = 0; col < 7; col++) {
                bool inside = cx >= colX(col) - GAP_X / 2 && cx <= colX(col) + CARD_W + GAP_X / 2;
                if (!inside) {
                    continue;
                }
                bool sameColumn = (dragType == TABLEAU_PILE && dragIdx == col);
                if (!sameColumn && canGoToTableau(first, col)) {
                    performMove(makeMove(CARD_MOVE, dragType, dragIdx, TABLEAU_PILE, col, dragCount));
                }
                break;
            }
        }
        dragCards.clear();
    }

    void update() {
        float dt = GetFrameTime();
        if (!won) {
            elapsed += dt;
        }
        if (hintTimer > 0) {
            hintTimer -= dt;
        }

        if (!dragging) {
            bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            if (IsKeyPressed(KEY_U) || (ctrl && IsKeyPressed(KEY_Z))) undo();
            if (IsKeyPressed(KEY_R) || (ctrl && IsKeyPressed(KEY_Y))) redo();
            if (IsKeyPressed(KEY_H)) findHint();
            if (IsKeyPressed(KEY_N)) newGame();
            if (IsKeyPressed(KEY_D)) toggleDrawMode();
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            handlePress();
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            handleRelease();
        }
    }

    void drawBackCard(float x, float y) {
        DrawRectangleRounded({x, y, CARD_W, CARD_H}, 0.1f, 6, RAYWHITE);
        DrawRectangleRounded({x + 5, y + 5, CARD_W - 10, CARD_H - 10}, 0.1f, 6, backBlue);
        DrawRectangleLinesEx({x + 12, y + 12, CARD_W - 24, CARD_H - 24}, 2, backLight);
        DrawCircle((int)(x + CARD_W / 2), (int)(y + CARD_H / 2), 14, backLight);
    }

    void drawCardAt(Card* c, float x, float y) {
        if (c->faceUp) {
            DrawTexture(faces[c->id()], (int)x, (int)y, WHITE);
            DrawRectangleLinesEx({x, y, CARD_W, CARD_H}, 1, cardEdge);
        } else {
            drawBackCard(x, y);
        }
    }

    void drawSlot(float x, float y, const char* label) {
        DrawRectangleRounded({x, y, CARD_W, CARD_H}, 0.1f, 6, slotShade);
        DrawRectangleLinesEx({x, y, CARD_W, CARD_H}, 2, slotLine);
        if (label[0] != '\0') {
            int w = MeasureText(label, 40);
            DrawText(label, (int)(x + CARD_W / 2 - w / 2), (int)(y + CARD_H / 2 - 20), 40, slotText);
        }
    }

    void drawHint() {
        if (hintTimer <= 0 || hintMode == 0) {
            return;
        }
        if (hintMode == 2) {
            DrawRectangleLinesEx({colX(0) - 3, TOP_Y - 3, CARD_W + 6, CARD_H + 6}, 4, hintGold);
            return;
        }

        float sx = 0;
        float sy = TOP_Y;
        float sh = CARD_H;
        if (hintSrcType == WASTE_PILE) {
            sx = wasteX(wasteShown() - 1);
        } else {
            sx = colX(hintSrcIdx);
            sy = tableauCardY(hintSrcIdx, hintSrcPos);
            sh = CARD_H + (tableau[hintSrcIdx].size() - hintSrcPos - 1) * UP_OFFSET;
        }
        DrawRectangleLinesEx({sx - 3, sy - 3, CARD_W + 6, sh + 6}, 4, hintGold);

        float dx = 0;
        float dy = TOP_Y;
        if (hintDstType == FOUNDATION_PILE) {
            dx = colX(3 + hintDstIdx);
        } else {
            dx = colX(hintDstIdx);
            dy = TABLEAU_Y;
            int n = tableau[hintDstIdx].size();
            if (n > 0) {
                dy = tableauCardY(hintDstIdx, n - 1);
            }
        }
        DrawRectangleLinesEx({dx - 3, dy - 3, CARD_W + 6, CARD_H + 6}, 4, hintGold);
    }

    void drawBoard() {
        ClearBackground(feltGreen);

        if (stock.isEmpty()) {
            drawSlot(colX(0), TOP_Y, waste.size() > 0 ? "R" : "");
        } else {
            drawBackCard(colX(0), TOP_Y);
        }
        DrawText(TextFormat("%d", stock.size()), (int)colX(0), (int)(TOP_Y + CARD_H + 6), 18, RAYWHITE);

        drawSlot(colX(1), TOP_Y, "");
        int shown = wasteShown();
        for (int s = 0; s < shown; s++) {
            if (dragging && dragType == WASTE_PILE && s == shown - 1) {
                continue;
            }
            drawCardAt(waste.at(waste.size() - shown + s), wasteX(s), TOP_Y);
        }

        const char* labels[4] = {"H", "D", "C", "S"};
        for (int i = 0; i < 4; i++) {
            float x = colX(3 + i);
            drawSlot(x, TOP_Y, labels[i]);
            bool beingDragged = dragging && dragType == FOUNDATION_PILE && dragIdx == i;
            if (!foundation[i].isEmpty() && !beingDragged) {
                drawCardAt(foundation[i].top(), x, TOP_Y);
            }
        }

        for (int col = 0; col < 7; col++) {
            int n = tableau[col].size();
            if (n == 0) {
                drawSlot(colX(col), TABLEAU_Y, "");
                continue;
            }
            float y = TABLEAU_Y;
            for (int pos = 0; pos < n; pos++) {
                Card* c = tableau[col].at(pos);
                bool hidden = dragging && dragType == TABLEAU_PILE && dragIdx == col && pos >= n - dragCount;
                if (!hidden) {
                    drawCardAt(c, colX(col), y);
                }
                y += c->faceUp ? UP_OFFSET : DOWN_OFFSET;
            }
        }

        drawHint();

        if (dragging) {
            Vector2 m = GetMousePosition();
            for (int k = 0; k < (int)dragCards.size(); k++) {
                drawCardAt(dragCards[k], m.x - dragOffX, m.y - dragOffY + k * UP_OFFSET);
            }
        }
    }

    void drawButton(Rectangle r, const char* label) {
        bool hover = CheckCollisionPointRec(GetMousePosition(), r);
        DrawRectangleRounded(r, 0.3f, 6, hover ? buttonHover : buttonColor);
        int w = MeasureText(label, 18);
        DrawText(label, (int)(r.x + (r.width - w) / 2), (int)(r.y + 6), 18, RAYWHITE);
    }

    void drawHud() {
        DrawRectangle(0, (int)BAR_Y, SCREEN_W, SCREEN_H - (int)BAR_Y, barColor);
        drawButton(buttons[0], "Undo");
        drawButton(buttons[1], "Redo");
        drawButton(buttons[2], "Hint");
        drawButton(buttons[3], "New Game");
        drawButton(buttons[4], TextFormat("Draw %d", drawCount));
        int secs = (int)elapsed;
        DrawText(TextFormat("Score %d     Moves %d     Time %02d:%02d", score, moves, secs / 60, secs % 60), 570, 871, 18, RAYWHITE);
    }

    void drawWinScreen() {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, dimmer);
        const char* title = "You Win!";
        DrawText(title, SCREEN_W / 2 - MeasureText(title, 80) / 2, 320, 80, hintGold);
        int secs = (int)elapsed;
        const char* line = TextFormat("Score %d   Moves %d   Time %02d:%02d", score, moves, secs / 60, secs % 60);
        DrawText(line, SCREEN_W / 2 - MeasureText(line, 28) / 2, 430, 28, RAYWHITE);
        const char* again = "Press N or click New Game to play again";
        DrawText(again, SCREEN_W / 2 - MeasureText(again, 22) / 2, 480, 22, RAYWHITE);
    }

    void draw() {
        drawBoard();
        drawHud();
        if (won) {
            drawWinScreen();
        }
    }
};

int main() {
    srand((unsigned int)time(NULL));
    InitWindow(SCREEN_W, SCREEN_H, "Klondike Solitaire");
    SetTargetFPS(60);

    if (!FileExists("card_images/ace_of_hearts.png")) {
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(feltGreen);
            DrawText("card_images folder not found.", 60, 340, 32, RAYWHITE);
            DrawText("Put it in the same folder as solitaire.exe and run again.", 60, 390, 24, RAYWHITE);
            EndDrawing();
        }
        CloseWindow();
        return 1;
    }

    {
        Game game;
        while (!WindowShouldClose()) {
            game.update();
            BeginDrawing();
            game.draw();
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}
