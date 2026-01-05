#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(_WIN32)
  #include <windows.h>
#endif
#include <GL/gl.h>

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;

  Vec2() = default;
  Vec2(float x_, float y_) : x(x_), y(y_) {}

  Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
  Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
  Vec2 operator*(float s) const { return {x * s, y * s}; }
  Vec2 operator/(float s) const { return {x / s, y / s}; }

  Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
  Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
  Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
};

static inline float dot(const Vec2& a, const Vec2& b) { return a.x*b.x + a.y*b.y; }
static inline float norm2(const Vec2& a) { return dot(a,a); }
static inline float norm(const Vec2& a) { return std::sqrt(norm2(a)); }
static inline Vec2 normalized(const Vec2& a) {
  float n = norm(a);
  if (n <= 1e-8f) return {0.0f, 0.0f};
  return a / n;
}

enum class Escena {
  Libre = 1,
  FuerzaConstante = 2,
  Oscilador = 3,
  InversaCuadrado = 4,
  RestriccionCirculo = 5,

  OsciladorRigidezTiempo = 6,
  NoetherRotacionAreal = 7,
  NoetherTraslacionPx = 8
};

enum class Integrador {
  EulerSemimplicito = 1,
  RK4 = 2
};

struct Estado {
  Vec2 r;
  Vec2 v;
};

struct Parametros {
  float m = 1.0f;
  float g = 1.5f;
  float k = 2.0f;
  float radio = 1.25f;
  float amortiguamiento = 0.15f;

  float eps_k = 0.35f;
  float omega_k = 2.5f;
};

struct Simulacion {
  Escena escena = Escena::FuerzaConstante;
  Integrador integrador = Integrador::EulerSemimplicito;

  Parametros p;
  Estado s;
  Vec2 a;
  float dt = 1.0f / 120.0f;
  bool pausado = false;

  std::vector<Vec2> estela;

  double t_sim = 0.0;

  Vec2 r_previa{0.0f, 0.0f};
  double area_acumulada = 0.0;
  double reloj_area = 0.0;

  double reloj_imprime = 0.0;
  int contador_frames = 0;

  void reiniciar() {
    s.r = { -0.8f, 0.6f };
    s.v = { 1.2f, 0.2f };
    a = {0.0f, 0.0f};
    estela.clear();

    t_sim = 0.0;
    r_previa = s.r;
    area_acumulada = 0.0;
    reloj_area = 0.0;
  }
};

static float rigidez_tiempo(double t, const Parametros& p) {
  return p.k * (1.0f + p.eps_k * std::sin((float)(p.omega_k * t)));
}

static Vec2 fuerza(const Estado& s, Escena escena, const Parametros& p, double t) {
  Vec2 F{0.0f, 0.0f};

  switch (escena) {
    case Escena::Libre:
      break;

    case Escena::FuerzaConstante:
      F.y += -p.m * p.g;
      F += s.v * (-p.amortiguamiento);
      break;

    case Escena::Oscilador:
      F += s.r * (-p.k);
      F += s.v * (-p.amortiguamiento);
      break;

    case Escena::InversaCuadrado: {
      float r2 = norm2(s.r);
      float r = std::sqrt(r2);
      float eps = 1e-4f;
      float denom = std::max(r2 * r, eps);
      F += s.r * (-p.k / denom);
      break;
    }

    case Escena::RestriccionCirculo:
      F += s.r * (-p.k);
      break;

    case Escena::OsciladorRigidezTiempo: {
      float kt = rigidez_tiempo(t, p);
      F += s.r * (-kt);
      break;
    }

    case Escena::NoetherRotacionAreal: {
      float r2 = norm2(s.r);
      float r = std::sqrt(r2);
      float eps = 1e-4f;
      float denom = std::max(r2 * r, eps);
      F += s.r * (-p.k / denom);
      break;
    }

    case Escena::NoetherTraslacionPx:
      F.y += -p.m * p.g;
      break;
  }
  return F;
}

static float energia_potencial(const Estado& s, Escena escena, const Parametros& p, double t) {
  switch (escena) {
    case Escena::Libre:
      return 0.0f;
    case Escena::FuerzaConstante:
      return p.m * p.g * s.r.y;
    case Escena::Oscilador:
      return 0.5f * p.k * norm2(s.r);
    case Escena::InversaCuadrado: {
      float r = std::max(norm(s.r), 1e-4f);
      return -p.k / r;
    }
    case Escena::RestriccionCirculo:
      return 0.5f * p.k * norm2(s.r);

    case Escena::OsciladorRigidezTiempo: {
      float kt = rigidez_tiempo(t, p);
      return 0.5f * kt * norm2(s.r);
    }

    case Escena::NoetherRotacionAreal: {
      float r = std::max(norm(s.r), 1e-4f);
      return -p.k / r;
    }

    case Escena::NoetherTraslacionPx:
      return p.m * p.g * s.r.y;
  }
  return 0.0f;
}

static void aplicar_restriccion_circulo(Estado& s, const Parametros& p) {
  float r = norm(s.r);
  if (r < 1e-6f) {
    s.r = {p.radio, 0.0f};
    s.v = {0.0f, 0.0f};
    return;
  }

  Vec2 n = s.r / r;
  s.r = n * p.radio;

  float v_rad = dot(s.v, n);
  s.v -= n * v_rad;
}

static Estado derivadas(const Estado& s, Escena escena, const Parametros& p, double t) {
  Vec2 F = fuerza(s, escena, p, t);
  Vec2 a = F / p.m;
  return Estado{ s.v, a };
}

static Estado suma(const Estado& a, const Estado& b) {
  return Estado{ a.r + b.r, a.v + b.v };
}

static Estado escala(const Estado& a, float s) {
  return Estado{ a.r * s, a.v * s };
}

static void paso_euler_semimplicito(Simulacion& sim) {
  Vec2 F = fuerza(sim.s, sim.escena, sim.p, sim.t_sim);
  sim.a = F / sim.p.m;
  sim.s.v += sim.a * sim.dt;
  sim.s.r += sim.s.v * sim.dt;

  if (sim.escena == Escena::RestriccionCirculo) {
    aplicar_restriccion_circulo(sim.s, sim.p);
  }

  sim.t_sim += sim.dt;
}

static void paso_rk4(Simulacion& sim) {
  double t0 = sim.t_sim;
  Estado k1 = derivadas(sim.s, sim.escena, sim.p, t0);
  Estado k2 = derivadas(suma(sim.s, escala(k1, sim.dt * 0.5f)), sim.escena, sim.p, t0 + sim.dt * 0.5);
  Estado k3 = derivadas(suma(sim.s, escala(k2, sim.dt * 0.5f)), sim.escena, sim.p, t0 + sim.dt * 0.5);
  Estado k4 = derivadas(suma(sim.s, escala(k3, sim.dt)), sim.escena, sim.p, t0 + sim.dt);

  Estado inc = suma(suma(k1, escala(k2, 2.0f)), suma(escala(k3, 2.0f), k4));
  sim.s.r += inc.r * (sim.dt / 6.0f);
  sim.s.v += inc.v * (sim.dt / 6.0f);

  Vec2 F = fuerza(sim.s, sim.escena, sim.p, t0 + sim.dt);
  sim.a = F / sim.p.m;

  if (sim.escena == Escena::RestriccionCirculo) {
    aplicar_restriccion_circulo(sim.s, sim.p);
    Vec2 F2 = fuerza(sim.s, sim.escena, sim.p, t0 + sim.dt);
    sim.a = F2 / sim.p.m;
  }

  sim.t_sim += sim.dt;
}

static const char* nombre_escena(Escena e) {
  switch (e) {
    case Escena::Libre: return "Libre";
    case Escena::FuerzaConstante: return "Fuerza constante";
    case Escena::Oscilador: return "Oscilador";
    case Escena::InversaCuadrado: return "Inversa al cuadrado";
    case Escena::RestriccionCirculo: return "Restriccion de circulo";
    case Escena::OsciladorRigidezTiempo: return "Oscilador con rigidez en el tiempo";
    case Escena::NoetherRotacionAreal: return "Noether rotacion y velocidad areolar";
    case Escena::NoetherTraslacionPx: return "Noether traslacion y px constante";
  }
  return "Desconocido";
}

static const char* nombre_integrador(Integrador i) {
  switch (i) {
    case Integrador::EulerSemimplicito: return "Euler semimplicito";
    case Integrador::RK4: return "Runge Kutta 4";
  }
  return "Desconocido";
}

static void imprimir_ayuda() {
  std::puts("");
  std::puts("Controles");
  std::puts("  1  Particula libre");
  std::puts("  2  Fuerza constante hacia abajo con arrastre");
  std::puts("  3  Oscilador armonico con arrastre");
  std::puts("  4  Fuerza central inversa al cuadrado tipo Kepler");
  std::puts("  5  Restriccion holonoma a un circulo y fuerza hacia el origen");
  std::puts("  6  Oscilador con rigidez dependiente del tiempo");
  std::puts("  7  Noether por rotacion en fuerza central, imprime velocidad areolar");
  std::puts("  8  Noether por traslacion en x, gravedad sin arrastre imprime px");
  std::puts("  Espacio  Pausa");
  std::puts("  R        Reiniciar");
  std::puts("  I        Cambiar integrador");
  std::puts("  Flecha arriba y abajo  Ajustar paso de tiempo");
  std::puts("  Flecha izquierda y derecha  Ajustar intensidad k");
  std::puts("  H        Mostrar esta ayuda");
  std::puts("");
  std::puts("Lectura visual");
  std::puts("  Punto blanco  Particula");
  std::puts("  Vector verde  Velocidad");
  std::puts("  Vector rojo   Aceleracion");
  std::puts("  Eje gris      Referencia");
  std::puts("  Estela azul   Trayectoria reciente");
  std::puts("");
}

static void configurar_ortho(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  float aspecto = (h > 0) ? (float)w / (float)h : 1.0f;
  float escala = 2.5f;
  glOrtho(-escala * aspecto, escala * aspecto, -escala, escala, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

static void dibujar_ejes(float ext) {
  glColor3f(0.35f, 0.35f, 0.35f);
  glBegin(GL_LINES);
    glVertex2f(-ext, 0.0f); glVertex2f(ext, 0.0f);
    glVertex2f(0.0f, -ext); glVertex2f(0.0f, ext);
  glEnd();
}

static void dibujar_circulo(float r, int segmentos, float cx=0.0f, float cy=0.0f) {
  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < segmentos; ++i) {
    float t = 2.0f * 3.1415926535f * (float)i / (float)segmentos;
    glVertex2f(cx + r * std::cos(t), cy + r * std::sin(t));
  }
  glEnd();
}

static void dibujar_punto(const Vec2& p, float radio) {
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(p.x, p.y);
  int seg = 24;
  for (int i = 0; i <= seg; ++i) {
    float t = 2.0f * 3.1415926535f * (float)i / (float)seg;
    glVertex2f(p.x + radio * std::cos(t), p.y + radio * std::sin(t));
  }
  glEnd();
}

static void dibujar_vector(const Vec2& desde, const Vec2& v, float escala) {
  Vec2 hasta = desde + v * escala;
  glBegin(GL_LINES);
    glVertex2f(desde.x, desde.y);
    glVertex2f(hasta.x, hasta.y);
  glEnd();

  Vec2 dir = normalized(hasta - desde);
  Vec2 izq{ -dir.y, dir.x };
  float punta = 0.08f;
  Vec2 p1 = hasta - dir * (punta * 1.2f) + izq * (punta * 0.6f);
  Vec2 p2 = hasta - dir * (punta * 1.2f) - izq * (punta * 0.6f);
  glBegin(GL_TRIANGLES);
    glVertex2f(hasta.x, hasta.y);
    glVertex2f(p1.x, p1.y);
    glVertex2f(p2.x, p2.y);
  glEnd();
}

static void actualizar_estela(Simulacion& sim) {
  sim.estela.push_back(sim.s.r);
  const std::size_t max_puntos = 900;
  if (sim.estela.size() > max_puntos) {
    sim.estela.erase(sim.estela.begin(), sim.estela.begin() + (sim.estela.size() - max_puntos));
  }
}

static void dibujar_estela(const std::vector<Vec2>& estela) {
  if (estela.size() < 2) return;
  glColor3f(0.25f, 0.55f, 0.95f);
  glBegin(GL_LINE_STRIP);
  for (const auto& p : estela) glVertex2f(p.x, p.y);
  glEnd();
}

static void imprimir_estado_si_toca(Simulacion& sim, double t_actual) {
  float T = 0.5f * sim.p.m * norm2(sim.s.v);
  float V = energia_potencial(sim.s, sim.escena, sim.p, sim.t_sim);
  float E = T + V;

  float Lz = sim.p.m * (sim.s.r.x * sim.s.v.y - sim.s.r.y * sim.s.v.x);

  float px = sim.p.m * sim.s.v.x;

  if (sim.reloj_imprime == 0.0) sim.reloj_imprime = t_actual;
  double dt = t_actual - sim.reloj_imprime;
  if (dt < 1.0) return;

  if (sim.escena == Escena::NoetherRotacionAreal) {
    if (sim.reloj_area == 0.0) sim.reloj_area = t_actual;
    double dt_area = t_actual - sim.reloj_area;
    double vel_areolar = (dt_area > 0.0) ? (sim.area_acumulada / dt_area) : 0.0;
    double vel_areolar_ideal = (double)Lz / (2.0 * (double)sim.p.m);

    std::printf(
      "Escena: %-28s | Integrador: %-18s | dt: %.5f | E: %.5f | Lz: %.5f | v_areal: %.6f | Lz/(2m): %.6f\n",
      nombre_escena(sim.escena),
      nombre_integrador(sim.integrador),
      sim.dt,
      E,
      Lz,
      vel_areolar,
      vel_areolar_ideal
    );
    sim.area_acumulada = 0.0;
    sim.reloj_area = t_actual;
    sim.reloj_imprime = t_actual;
    return;
  }

  if (sim.escena == Escena::NoetherTraslacionPx) {
    std::printf(
      "Escena: %-28s | Integrador: %-18s | dt: %.5f | E: %.5f | px: %.5f | r: (%.3f, %.3f) | v: (%.3f, %.3f)\n",
      nombre_escena(sim.escena),
      nombre_integrador(sim.integrador),
      sim.dt,
      E,
      px,
      sim.s.r.x, sim.s.r.y,
      sim.s.v.x, sim.s.v.y
    );
    sim.reloj_imprime = t_actual;
    return;
  }

  if (sim.escena == Escena::OsciladorRigidezTiempo) {
    float kt = rigidez_tiempo(sim.t_sim, sim.p);
    std::printf(
      "Escena: %-28s | Integrador: %-18s | dt: %.5f | k(t): %.5f | E: %.5f | r: (%.3f, %.3f) | v: (%.3f, %.3f)\n",
      nombre_escena(sim.escena),
      nombre_integrador(sim.integrador),
      sim.dt,
      kt,
      E,
      sim.s.r.x, sim.s.r.y,
      sim.s.v.x, sim.s.v.y
    );
    sim.reloj_imprime = t_actual;
    return;
  }

  std::printf("Escena: %-22s | Integrador: %-18s | dt: %.5f | E: %.5f | Lz: %.5f | r: (%.3f, %.3f) | v: (%.3f, %.3f)\n",
    nombre_escena(sim.escena),
    nombre_integrador(sim.integrador),
    sim.dt,
    E,
    Lz,
    sim.s.r.x, sim.s.r.y,
    sim.s.v.x, sim.s.v.y
  );

  sim.reloj_imprime = t_actual;
}

static void aplicar_cambios_por_tecla(Simulacion& sim, int key) {
  if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
    sim.escena = (Escena)(key - GLFW_KEY_0);
    sim.reiniciar();

    if (sim.escena == Escena::NoetherTraslacionPx) {
      sim.p.amortiguamiento = 0.0f;
    }
    if (sim.escena == Escena::NoetherRotacionAreal) {
      sim.p.amortiguamiento = 0.0f;
    }
    if (sim.escena == Escena::OsciladorRigidezTiempo) {
      sim.p.amortiguamiento = 0.0f;
    }

    std::printf("Escena seleccionada: %s\n", nombre_escena(sim.escena));
  }

  if (key == GLFW_KEY_I) {
    sim.integrador = (sim.integrador == Integrador::EulerSemimplicito) ? Integrador::RK4 : Integrador::EulerSemimplicito;
    std::printf("Integrador: %s\n", nombre_integrador(sim.integrador));
  }

  if (key == GLFW_KEY_R) {
    sim.reiniciar();
    std::puts("Reinicio.");
  }

  if (key == GLFW_KEY_SPACE) {
    sim.pausado = !sim.pausado;
    std::puts(sim.pausado ? "Pausa." : "Continuar.");
  }

  if (key == GLFW_KEY_H) {
    imprimir_ayuda();
  }

  if (key == GLFW_KEY_UP) {
    sim.dt *= 1.10f;
    sim.dt = std::min(sim.dt, 1.0f / 15.0f);
  }
  if (key == GLFW_KEY_DOWN) {
    sim.dt *= 0.90f;
    sim.dt = std::max(sim.dt, 1.0f / 600.0f);
  }

  if (key == GLFW_KEY_RIGHT) {
    sim.p.k *= 1.10f;
    sim.p.k = std::min(sim.p.k, 50.0f);
  }
  if (key == GLFW_KEY_LEFT) {
    sim.p.k *= 0.90f;
    sim.p.k = std::max(sim.p.k, 0.02f);
  }
}

static void callback_teclado(GLFWwindow* w, int key, int scancode, int action, int mods) {
  (void)scancode;
  (void)mods;
  if (action != GLFW_PRESS) return;

  auto* sim = reinterpret_cast<Simulacion*>(glfwGetWindowUserPointer(w));
  if (!sim) return;
  aplicar_cambios_por_tecla(*sim, key);
}

int main() {
  if (!glfwInit()) {
    std::fprintf(stderr, "Fallo al iniciar GLFW\n");
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  GLFWwindow* window = glfwCreateWindow(1100, 700, "Goldstein Capitulos 1 y 2: particula, simetrias y conservaciones", nullptr, nullptr);
  if (!window) {
    std::fprintf(stderr, "No se pudo crear ventana\n");
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  Simulacion sim;
  sim.reiniciar();

  glfwSetWindowUserPointer(window, &sim);
  glfwSetKeyCallback(window, callback_teclado);

  glDisable(GL_DEPTH_TEST);
  glLineWidth(2.0f);

  imprimir_ayuda();

  double t_anterior = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    double t_actual = glfwGetTime();
    double dt_real = t_actual - t_anterior;
    t_anterior = t_actual;

    if (!sim.pausado) {
      double acumulado = dt_real;
      double paso = sim.dt;
      int iter = 0;
      while (acumulado > 0.0 && iter < 120) {
        double h = std::min(acumulado, paso);
        sim.dt = (float)h;

        if (sim.integrador == Integrador::EulerSemimplicito) paso_euler_semimplicito(sim);
        else paso_rk4(sim);

        if (sim.escena == Escena::NoetherRotacionAreal) {
          Vec2 r0 = sim.r_previa;
          Vec2 r1 = sim.s.r;
          double cruz = (double)r0.x * (double)r1.y - (double)r0.y * (double)r1.x;
          sim.area_acumulada += 0.5 * std::fabs(cruz);
          sim.r_previa = r1;
        }

        actualizar_estela(sim);
        acumulado -= h;
        iter++;
      }
    }

    imprimir_estado_si_toca(sim, t_actual);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    configurar_ortho(w, h);

    glClearColor(0.05f, 0.05f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    dibujar_ejes(10.0f);

    if (sim.escena == Escena::RestriccionCirculo) {
      glColor3f(0.55f, 0.55f, 0.55f);
      dibujar_circulo(sim.p.radio, 96);
    }

    dibujar_estela(sim.estela);

    glColor3f(0.95f, 0.95f, 0.95f);
    dibujar_punto(sim.s.r, 0.06f);

    glColor3f(0.25f, 0.95f, 0.35f);
    dibujar_vector(sim.s.r, sim.s.v, 0.35f);

    glColor3f(0.95f, 0.25f, 0.25f);
    dibujar_vector(sim.s.r, sim.a, 0.20f);

    glfwSwapBuffers(window);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
