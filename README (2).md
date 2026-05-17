# 🔥 The Silent Cylinder v2.0

A 2D real-time interactive game built with **C++ and OpenGL (GLUT)**, simulating a kitchen gas leak emergency scenario. The player must escape the blast, pick up a fire extinguisher, and put out all fires to win.

Built as a coursework project at **East West University**, Department of Computer Science & Engineering.

---

## 🎮 Gameplay

The game unfolds in four story stages:

| Stage | Description |
|-------|-------------|
| **Calm** | Kitchen appears normal, gas slowly leaks from the cylinder |
| **Hissing** | Visible gas cloud appears, sparks begin flying |
| **Warning** | Alarm flashes, cylinder shakes — run! |
| **Fire** | Explosion occurs, fires spread — grab the extinguisher and fight back |

**Controls:**
- `A` / `←` — Move left
- `D` / `→` — Move right
- `Space` — Shoot foam (once extinguisher is picked up)
- `Enter` — Start / Restart game

**Win condition:** Pick up the fire extinguisher and put out all fires before taking 10 burns.

---

## 🛠️ Technical Highlights

- **Game state machine** — 6 states (Menu, Play, Boom, Mission Complete, Game Over, Win) managed with enums and clean transitions
- **Particle systems** — Gas puffs, sparks, foam projectiles, and confetti each implemented as independent particle arrays with spawn/update/destroy logic
- **Collision detection** — Distance-based collision between player, fires, foam, extinguisher, and mosquito
- **Real-time animation** — 60 FPS timer loop using `glutTimerFunc`, screen shake on explosion, fire flicker with sine-wave intensity
- **Story-driven progression** — Timed stage transitions drive the narrative automatically

---

## 📸 Screenshots

> *(Add screenshots here after running the game)*

---

## ⚙️ How to Run

### Requirements
- Windows OS
- Code::Blocks with MinGW
- FreeGLUT library

### Setup
1. Clone the repository:
   ```bash
   git clone https://github.com/Arifool/silent-cylinder.git
   ```
2. Open `silent_cylinder_v2.cpp` in Code::Blocks
3. Set linker flags:
   ```
   -lfreeglut -lopengl32 -lglu32
   ```
4. Build and Run (`F9`)

---

## 📚 Concepts Demonstrated

- C++ (OOP, structs, arrays, math functions)
- OpenGL 2D rendering (quads, polygons, blending, alpha transparency)
- Real-time game loop design
- Finite state machine architecture
- Particle system design
- Event-driven input handling

---

## 👨‍💻 Author

**Md Ariful Islam**  
B.Sc. in Computer Science & Engineering (Data Science Major)  
East West University, Dhaka  
[github.com/Arifool](https://github.com/Arifool) | [arifool.github.io](https://arifool.github.io)
