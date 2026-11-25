# Plik: Tools/podglad.py
# Podglad glb bez Unreala: rzut ortograficzny z gory pod katem 38 st., slonce + cien (shadow map), tekstury, potem pikselizacja.
# Uzywany do arkuszy podgladu faz (python Tools/podglad.py SM_Kosciol) i do ikon paska budowy (Tools/ikony.py).
import json, struct, sys, os
import numpy as np
from PIL import Image

D = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # katalog projektu (plik lezy w Tools/)
ART = os.path.join(D, "Art", "Budynki")
TEX = {0: "T_Bud_Kamien", 1: "T_Bud_Dach", 2: "T_Bud_Drewno", 3: "T_Bud_Tynk", 4: "T_Bud_Strzecha"}
TEXI = {k: np.asarray(Image.open(os.path.join(ART, v + ".png")).convert("RGB")).astype(np.float32) / 255 for k, v in TEX.items()}


def wczytaj(plik):
    b = open(plik, "rb").read()
    jl = struct.unpack("<I", b[12:16])[0]
    js = json.loads(b[20:20 + jl])
    bin_ = b[20 + jl + 8:]
    def acc(i):
        a = js["accessors"][i]; v = js["bufferViews"][a["bufferView"]]
        n = {"VEC3": 3, "VEC4": 4, "VEC2": 2, "SCALAR": 1}[a["type"]]
        dt = np.float32 if a["componentType"] == 5126 else np.uint32
        return np.frombuffer(bin_, dt, a["count"] * n, v["byteOffset"]).reshape(a["count"], n)
    out = []
    for p in js["meshes"][0]["primitives"]:
        A = p["attributes"]
        P = acc(A["POSITION"]).astype(float); P = np.stack([P[:, 0], -P[:, 2], P[:, 1]], 1)  # z powrotem Z-gora
        N = acc(A["NORMAL"]).astype(float); N = np.stack([N[:, 0], -N[:, 2], N[:, 1]], 1)
        out.append((p["material"], P, N, acc(A["COLOR_0"]).copy(), acc(A["TEXCOORD_0"]), acc(p["indices"]).ravel().reshape(-1, 3)))
    return out


def macierz(yaw, pitch):
    y, p = np.radians(yaw), np.radians(pitch)
    f = np.array([np.cos(p) * np.cos(y), np.cos(p) * np.sin(y), -np.sin(p)])  # kierunek patrzenia
    r = np.cross(f, [0, 0, 1.0]); r /= np.linalg.norm(r)
    u = np.cross(r, f)
    return np.stack([r, u, -f])  # x ekranu, y ekranu, glebia (w strone kamery)


def rasteryzuj(tris_xy, tris_z, W, H, cb=None):
    """tris_xy: (T,3,2) w pikselach; zwraca zbufor + indeks trojkata + wspolrzedne barycentryczne."""
    Z = np.full((H, W), -1e9); I = np.full((H, W), -1, int); B = np.zeros((H, W, 3))
    for t in range(len(tris_xy)):
        (x0, y0), (x1, y1), (x2, y2) = tris_xy[t]
        mnx, mxx = int(max(0, np.floor(min(x0, x1, x2)))), int(min(W - 1, np.ceil(max(x0, x1, x2))))
        mny, mxy = int(max(0, np.floor(min(y0, y1, y2)))), int(min(H - 1, np.ceil(max(y0, y1, y2))))
        if mnx > mxx or mny > mxy:
            continue
        den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
        if abs(den) < 1e-9:
            continue
        xs, ys = np.meshgrid(np.arange(mnx, mxx + 1) + 0.5, np.arange(mny, mxy + 1) + 0.5)
        l0 = ((y1 - y2) * (xs - x2) + (x2 - x1) * (ys - y2)) / den
        l1 = ((y2 - y0) * (xs - x2) + (x0 - x2) * (ys - y2)) / den
        l2 = 1 - l0 - l1
        m = (l0 >= -1e-6) & (l1 >= -1e-6) & (l2 >= -1e-6)
        z = l0 * tris_z[t, 0] + l1 * tris_z[t, 1] + l2 * tris_z[t, 2]
        sub = Z[mny:mxy + 1, mnx:mxx + 1]
        m &= z > sub
        sub[m] = z[m]
        I[mny:mxy + 1, mnx:mxx + 1][m] = t
        B[mny:mxy + 1, mnx:mxx + 1][m] = np.stack([l0, l1, l2], -1)[m]
    return Z, I, B


def renderuj(plik, W=520, H=400, skala=13.0, yaw=60, pitch=38, czesci=None, g=30.0, cien=70.0, ziemia=True, dopasuj=False, pikselizuj=True, obetnij_z=None, odrzuc_tyl=False):
    """ziemia=False: tlo przezroczyste (RGBA). dopasuj=True: skala i srodek tak, by model wypelnil kadr (ikony)."""
    czesci = wczytaj(plik) if czesci is None else czesci
    P = np.concatenate([c[1] for c in czesci])
    if obetnij_z is not None:
        P = P.copy(); P[:, 2] = np.maximum(P[:, 2], obetnij_z)  # ikony: podmurowka siegajaca 3 m w ziemie splaszczona do gruntu
    Nn = np.concatenate([c[2] for c in czesci]); C = np.concatenate([c[3][:, :3] for c in czesci])
    UV = np.concatenate([c[4] for c in czesci])
    Mt = np.concatenate([np.full(len(c[1]), c[0]) for c in czesci])
    baza = np.cumsum([0] + [len(c[1]) for c in czesci])[:-1]
    T = np.concatenate([c[5] + b for c, b in zip(czesci, baza)])
    # ziemia: duzy kwadrat
    gi = len(P)
    P = np.concatenate([P, [[-g, -g, 0], [g, -g, 0], [g, g, 0], [-g, g, 0]]])
    Nn = np.concatenate([Nn, [[0, 0, 1]] * 4]); C = np.concatenate([C, [[0.08, 0.13, 0.045]] * 4]); UV = np.concatenate([UV, np.zeros((4, 2))])
    Mt = np.concatenate([Mt, [-1] * 4])
    T = np.concatenate([T, [[gi, gi + 1, gi + 2], [gi, gi + 2, gi + 3]]])

    # slonce jak w regionie: 45 st. wysokosci, z lewej gory ekranu
    sl = np.array([np.cos(np.radians(45)) * np.cos(np.radians(yaw + 140)), np.cos(np.radians(45)) * np.sin(np.radians(yaw + 140)), np.sin(np.radians(45))])
    # mapa cieni
    ML = macierz(np.degrees(np.arctan2(-sl[1], -sl[0])), 45)
    PL = P @ ML.T
    SW = 900; ss = SW / cien
    xy = PL[:, :2] * ss + SW / 2
    ZS, _, _ = rasteryzuj(xy[T], PL[:, 2][T], SW, SW)

    M = macierz(yaw, pitch)
    PC = P @ M.T
    if not ziemia:
        T = T[:-2]  # bez kwadratu ziemi (cien liczony z nim nie przeszkadza)
    if odrzuc_tyl:  # jak w grze: material jednostronny (podglad ruin z odwroconymi scianami)
        T = T[np.einsum("ij,j->i", np.cross(P[T[:, 1]] - P[T[:, 0]], P[T[:, 2]] - P[T[:, 0]]), M[2]) > 0]
    if dopasuj:
        Pm = PC[:gi]
        mn, mx = Pm[:, :2].min(0), Pm[:, :2].max(0)
        skala = 0.9 * min(W / (mx[0] - mn[0]), H / (mx[1] - mn[1]))
        c = (mn + mx) / 2
        xy = np.stack([(PC[:, 0] - c[0]) * skala + W / 2, -(PC[:, 1] - c[1]) * skala + H / 2], 1)
    else:
        xy = np.stack([PC[:, 0] * skala + W / 2, -PC[:, 1] * skala + H * 0.62], 1)
    Z, I, B = rasteryzuj(xy[T], PC[:, 2][T], W, H)
    img = np.zeros((H, W, 3)) + [0.08, 0.13, 0.045]
    ys, xs = np.nonzero(I >= 0)
    t = I[ys, xs]; b = B[ys, xs]
    tri = T[t]
    pw = (P[tri] * b[..., None]).sum(1)
    nw = Nn[tri[:, 0]]
    col = (C[tri] * b[..., None]).sum(1)
    uv = (UV[tri] * b[..., None]).sum(1)
    mt = Mt[tri[:, 0]]
    tex = np.ones_like(col)
    for k, tx in TEXI.items():
        m = mt == k
        h, w = tx.shape[:2]
        u = (np.floor(np.mod(uv[m, 0], 1) * w)).astype(int) % w; v = (np.floor(np.mod(uv[m, 1], 1) * h)).astype(int) % h  # mod 1 daje czasem 1.0
        tex[m] = tx[v, u] ** 2.2
    # cien
    pl = pw @ ML.T
    sx = np.clip((pl[:, 0] * ss + SW / 2).astype(int), 0, SW - 1); sy = np.clip((pl[:, 1] * ss + SW / 2).astype(int), 0, SW - 1)
    oswietlony = pl[:, 2] >= ZS[sy, sx] - 0.08
    ndl = np.clip(nw @ sl, 0, 1) * oswietlony
    kolor = col * tex * (ndl[:, None] * 3.0 + 0.9)
    img[ys, xs] = kolor
    img = np.clip(img, 0, 1) ** (1 / 2.2)
    if ziemia:
        im = Image.fromarray((img * 255).astype(np.uint8))
    else:
        alfa = (I >= 0).astype(np.float32)[..., None]
        im = Image.fromarray((np.concatenate([img, alfa], -1) * 255).astype(np.uint8), "RGBA")
    if pikselizuj:
        # efekt pikseli: 50% i powrot bez wygladzania
        im = im.resize((W // 2, H // 2), Image.BILINEAR).resize((W, H), Image.NEAREST)
    return im


if __name__ == "__main__":
    nazwa = sys.argv[1] if len(sys.argv) > 1 else "SM_Kosciol"
    yaw = float(sys.argv[2]) if len(sys.argv) > 2 else 60
    fazy = 1
    while os.path.exists(os.path.join(ART, f"{nazwa}_F{fazy + 1}.glb")):
        fazy += 1
    obrazy = [renderuj(os.path.join(ART, f"{nazwa}_F{f}.glb"), yaw=yaw) for f in range(1, fazy + 1)]
    W, H = obrazy[0].size
    arkusz = Image.new("RGB", (W * 3, H * 2), (20, 30, 15))
    for k, im in enumerate(obrazy):
        arkusz.paste(im, ((k % 3) * W, (k // 3) * H))
    arkusz.save(os.path.join(D, "Saved", f"podglad_{nazwa}.png"))
    print("ok")
