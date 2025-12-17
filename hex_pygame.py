import math
import random
import time
from collections import deque

import pygame


# ----------------------------
# Настройки игры
# ----------------------------
N = 9                      # размер доски (9..11 комфортно по скорости ИИ)
HEX_SIZE = 30              # радиус шестиугольника (пиксели)
PADDING = 40

AI_TIME_LIMIT = 1.2        # секунд на ход ИИ
AI_EXPLORATION_C = 1.35    # константа UCT


# Цвета
BG = (20, 20, 24)
GRID = (60, 60, 70)
EMPTY_FILL = (30, 30, 38)

RED = (220, 60, 70)        # человек: соединяет ЛЕВО-ПРАВО
BLUE = (80, 140, 230)      # ИИ: соединяет ВЕРХ-НИЗ

TEXT = (230, 230, 235)
MUTED = (165, 165, 175)
PANEL = (28, 28, 35)


# Игроки
EMPTY = 0
P1 = 1   # RED (human)
P2 = 2   # BLUE (ai)


def other(p: int) -> int:
    return P1 if p == P2 else P2


# ----------------------------
# Геометрия Hex (pointy-top)
# axial coords: (q=c, r=row)
# x = s*sqrt(3)*(q + r/2), y = s*3/2*r
# ----------------------------
def hex_center(q, r, s):
    x = s * math.sqrt(3) * (q + r / 2.0)
    y = s * 1.5 * r
    return x, y


def hex_corners(cx, cy, s):
    pts = []
    for i in range(6):
        ang = math.radians(60 * i - 30)
        pts.append((cx + s * math.cos(ang), cy + s * math.sin(ang)))
    return pts


def point_in_poly(x, y, poly):
    # Ray casting
    inside = False
    n = len(poly)
    j = n - 1
    for i in range(n):
        xi, yi = poly[i]
        xj, yj = poly[j]
        intersect = ((yi > y) != (yj > y)) and (x < (xj - xi) * (y - yi) / ((yj - yi) + 1e-12) + xi)
        if intersect:
            inside = not inside
        j = i
    return inside


def idx_of(r, c):
    return r * N + c


def rc_of(idx):
    return divmod(idx, N)


def neighbors(r, c):
    # (q, r) neighbors for pointy-top axial mapping if we treat board as parallelogram
    # Using offset in (row=r, col=c) consistent with (q=c, r=row).
    dirs = [(0, -1), (0, 1), (-1, 0), (1, 0), (-1, 1), (1, -1)]
    for dc, dr in dirs:
        rr = r + dr
        cc = c + dc
        if 0 <= rr < N and 0 <= cc < N:
            yield rr, cc


# ----------------------------
# Проверка победы (BFS по цепочке)
# RED: соединить col=0 -> col=N-1
# BLUE: соединить row=0 -> row=N-1
# ----------------------------
def check_win(board, player):
    seen = set()
    q = deque()

    if player == P1:
        for r in range(N):
            if board[idx_of(r, 0)] == P1:
                q.append((r, 0))
                seen.add((r, 0))
        goal_col = N - 1
        while q:
            r, c = q.popleft()
            if c == goal_col:
                return True
            for rr, cc in neighbors(r, c):
                if (rr, cc) not in seen and board[idx_of(rr, cc)] == P1:
                    seen.add((rr, cc))
                    q.append((rr, cc))
        return False

    else:
        for c in range(N):
            if board[idx_of(0, c)] == P2:
                q.append((0, c))
                seen.add((0, c))
        goal_row = N - 1
        while q:
            r, c = q.popleft()
            if r == goal_row:
                return True
            for rr, cc in neighbors(r, c):
                if (rr, cc) not in seen and board[idx_of(rr, cc)] == P2:
                    seen.add((rr, cc))
                    q.append((rr, cc))
        return False


# ----------------------------
# DSU для быстрого определения победителя (для симуляций MCTS)
# ----------------------------
class DSU:
    def __init__(self, n):
        self.p = list(range(n))
        self.r = [0] * n

    def find(self, a):
        while self.p[a] != a:
            self.p[a] = self.p[self.p[a]]
            a = self.p[a]
        return a

    def union(self, a, b):
        ra = self.find(a)
        rb = self.find(b)
        if ra == rb:
            return
        if self.r[ra] < self.r[rb]:
            ra, rb = rb, ra
        self.p[rb] = ra
        if self.r[ra] == self.r[rb]:
            self.r[ra] += 1


def dsu_win(board, player):
    # Виртуальные узлы
    base = N * N
    a = base
    b = base + 1
    dsu = DSU(base + 2)

    for r in range(N):
        for c in range(N):
            if board[idx_of(r, c)] != player:
                continue
            v = idx_of(r, c)

            # границы
            if player == P1:
                if c == 0:
                    dsu.union(v, a)
                if c == N - 1:
                    dsu.union(v, b)
            else:
                if r == 0:
                    dsu.union(v, a)
                if r == N - 1:
                    dsu.union(v, b)

            # соседи
            for rr, cc in neighbors(r, c):
                if board[idx_of(rr, cc)] == player:
                    dsu.union(v, idx_of(rr, cc))

    return dsu.find(a) == dsu.find(b)


def winner_on_full(board):
    # На полностью заполненной доске победитель ровно один.
    if dsu_win(board, P1):
        return P1
    return P2


# ----------------------------
# MCTS (UCT)
# ----------------------------
class Node:
    __slots__ = ("parent", "move", "player_just_moved", "player_to_move",
                 "state", "children", "untried", "wins", "visits")

    def __init__(self, state, player_to_move, parent=None, move=None, player_just_moved=None):
        self.parent = parent
        self.move = move
        self.player_just_moved = player_just_moved
        self.player_to_move = player_to_move
        self.state = state  # tuple length N*N
        self.children = []
        self.untried = None
        self.wins = 0.0
        self.visits = 0

    def legal_moves(self):
        if self.untried is None:
            empties = [i for i, v in enumerate(self.state) if v == EMPTY]
            # Лёгкая эвристика порядка: ближе к центру — раньше
            cx = (N - 1) / 2.0
            cy = (N - 1) / 2.0

            def key(i):
                r, c = rc_of(i)
                return (abs(r - cy) + abs(c - cx))

            empties.sort(key=key)
            self.untried = empties
        return self.untried

    def uct_select_child(self, c=1.4):
        logv = math.log(self.visits + 1)
        best = None
        best_score = -1e9
        for ch in self.children:
            if ch.visits == 0:
                score = 1e9
            else:
                exploit = ch.wins / ch.visits
                explore = c * math.sqrt(logv / ch.visits)
                score = exploit + explore
            if score > best_score:
                best_score = score
                best = ch
        return best

    def add_child(self, move, new_state, next_player):
        ch = Node(
            state=new_state,
            player_to_move=next_player,
            parent=self,
            move=move,
            player_just_moved=self.player_to_move
        )
        self.children.append(ch)
        # удалить move из untried
        if self.untried is not None:
            self.untried.remove(move)
        return ch


def apply_move(state_tup, move_idx, player):
    st = list(state_tup)
    st[move_idx] = player
    return tuple(st)


def random_playout(state_tup, player_to_move):
    board = list(state_tup)
    empties = [i for i, v in enumerate(board) if v == EMPTY]
    random.shuffle(empties)
    p = player_to_move
    for mv in empties:
        board[mv] = p
        p = other(p)
    return winner_on_full(board)


def mcts_best_move(state_tup, player_to_move, time_limit=1.0, exploration_c=1.4):
    root = Node(state=state_tup, player_to_move=player_to_move)

    t0 = time.perf_counter()
    iters = 0

    while (time.perf_counter() - t0) < time_limit:
        node = root
        state = root.state
        p_to_move = root.player_to_move

        # Selection
        while node.legal_moves() == [] and node.children:
            node = node.uct_select_child(exploration_c)
            state = node.state
            p_to_move = node.player_to_move

        # Expansion
        untried = node.legal_moves()
        if untried:
            mv = untried[0]
            new_state = apply_move(state, mv, p_to_move)
            node = node.add_child(mv, new_state, other(p_to_move))
            state = new_state
            p_to_move = node.player_to_move

        # Simulation
        w = random_playout(state, p_to_move)

        # Backprop
        while node is not None:
            node.visits += 1
            if node.player_just_moved is not None and w == node.player_just_moved:
                node.wins += 1.0
            node = node.parent

        iters += 1

    # лучший ход по посещениям (обычно стабильнее, чем по winrate)
    if not root.children:
        return None
    root.children.sort(key=lambda ch: ch.visits, reverse=True)
    return root.children[0].move


# ----------------------------
# Pygame UI
# ----------------------------
def build_board_geometry():
    # сырой набор центров
    centers = []
    for r in range(N):
        for c in range(N):
            x, y = hex_center(c, r, HEX_SIZE)
            centers.append((x, y))

    minx = min(x for x, _ in centers)
    maxx = max(x for x, _ in centers)
    miny = min(y for _, y in centers)
    maxy = max(y for _, y in centers)

    shiftx = -minx + PADDING
    shifty = -miny + PADDING + 70  # сверху место под текст/панель

    centers2 = []
    polys = []
    for i, (x, y) in enumerate(centers):
        cx = x + shiftx
        cy = y + shifty
        centers2.append((cx, cy))
        polys.append(hex_corners(cx, cy, HEX_SIZE - 1))

    w = int((maxx - minx) + 2 * PADDING + HEX_SIZE * 2)
    h = int((maxy - miny) + 2 * PADDING + HEX_SIZE * 2 + 90)
    return (w, h), centers2, polys


def draw_text_lines(screen, font, lines, x, y, color=TEXT, line_h=26):
    yy = y
    for ln in lines:
        surf = font.render(ln, True, color)
        screen.blit(surf, (x, yy))
        yy += line_h


def main():
    pygame.init()
    pygame.display.set_caption("Hex (Pygame) — Human vs MCTS AI")

    (W, H), centers, polys = build_board_geometry()
    screen = pygame.display.set_mode((W, H))
    clock = pygame.time.Clock()

    font = pygame.font.SysFont("consolas", 20)
    font_big = pygame.font.SysFont("consolas", 28, bold=True)

    def reset():
        return [EMPTY] * (N * N), P1, "rules", None  # board, turn, mode, winner

    board, turn, mode, win = reset()

    def cell_at_pos(pos):
        x, y = pos
        for i, poly in enumerate(polys):
            if point_in_poly(x, y, poly):
                return i
        return None

    def draw_board():
        screen.fill(BG)

        # верхняя панель
        pygame.draw.rect(screen, PANEL, pygame.Rect(0, 0, W, 60))

        if mode == "rules":
            title = font_big.render("HEX — правила (нажми Enter/Space)", True, TEXT)
            screen.blit(title, (PADDING, 16))

            rules = [
                "1) Игроки ходят по очереди и ставят камень в пустую клетку.",
                "2) Камни не двигаются и не снимаются.",
                "3) Красный (ты) соединяет ЛЕВУЮ и ПРАВУЮ стороны.",
                "4) Синий (ИИ) соединяет ВЕРХНЮЮ и НИЖНЮЮ стороны.",
                "5) Побеждает тот, кто первым соберёт непрерывную цепочку.",
                "",
                "Управление: ЛКМ — ход, R — рестарт, Esc — выход.",
                f"Настройки: N={N}, время ИИ ~ {AI_TIME_LIMIT:.1f}с/ход",
            ]
            draw_text_lines(screen, font, rules, PADDING, 90, color=TEXT, line_h=26)
            return

        # текст статуса
        if win is None:
            if turn == P1:
                status = "Ход: ты (КРАСНЫЙ). ЛКМ по клетке."
                col = RED
            else:
                status = "Ход: ИИ (СИНИЙ) думает..."
                col = BLUE
        else:
            status = "ПОБЕДА: КРАСНЫЙ (ты)!" if win == P1 else "ПОБЕДА: СИНИЙ (ИИ)!"
            col = RED if win == P1 else BLUE

        screen.blit(font.render(status, True, col), (PADDING, 18))
        screen.blit(font.render("R — рестарт | Esc — выход", True, MUTED), (W - 320, 20))

        # рисуем клетки
        for i, poly in enumerate(polys):
            pygame.draw.polygon(screen, EMPTY_FILL, poly)
            pygame.draw.polygon(screen, GRID, poly, width=2)

            v = board[i]
            if v != EMPTY:
                cx, cy = centers[i]
                color = RED if v == P1 else BLUE
                pygame.draw.circle(screen, color, (int(cx), int(cy)), int(HEX_SIZE * 0.55))

        # метки сторон
        # Red: left/right
        screen.blit(font.render("RED", True, RED), (10, H // 2))
        screen.blit(font.render("RED", True, RED), (W - 60, H // 2))
        # Blue: top/bottom
        screen.blit(font.render("BLUE", True, BLUE), (W // 2 - 30, 64))
        screen.blit(font.render("BLUE", True, BLUE), (W // 2 - 30, H - 26))

    running = True
    while running:
        dt = clock.tick(60)

        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                running = False

            if ev.type == pygame.KEYDOWN:
                if ev.key == pygame.K_ESCAPE:
                    running = False
                if ev.key == pygame.K_r:
                    board, turn, mode, win = reset()
                    mode = "play"
                if mode == "rules" and ev.key in (pygame.K_RETURN, pygame.K_SPACE):
                    mode = "play"

            if mode == "play" and win is None and turn == P1:
                if ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                    mv = cell_at_pos(ev.pos)
                    if mv is not None and board[mv] == EMPTY:
                        board[mv] = P1
                        if check_win(board, P1):
                            win = P1
                        else:
                            turn = P2

        # Ход ИИ
        if mode == "play" and win is None and turn == P2:
            # обновить экран, чтобы было видно "думает..."
            draw_board()
            pygame.display.flip()

            state = tuple(board)
            mv = mcts_best_move(
                state_tup=state,
                player_to_move=P2,
                time_limit=AI_TIME_LIMIT,
                exploration_c=AI_EXPLORATION_C
            )
            if mv is None:
                # теоретически не должно случиться
                turn = P1
            else:
                board[mv] = P2
                if check_win(board, P2):
                    win = P2
                else:
                    turn = P1

        draw_board()
        pygame.display.flip()

    pygame.quit()


if __name__ == "__main__":
    main()
