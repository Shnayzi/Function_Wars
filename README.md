# 🎮 Function Wars
> **Projeto Integrador** — Jogo Educativo 2D desenvolvido em linguagem C com a biblioteca Allegro 5.
---
## 📖 Sobre o Projeto
Este projeto consiste em um jogo educativo interativo que visa reforçar conceitos de **matemática aplicada** (como funções de 1º e 2º grau, trigonometria e plano cartesiano) de forma lúdica e visual.
O jogador controla um personagem em um universo com estética de **caderno de rascunho (pencil sketch platformer)**, onde o objetivo é derrotar inimigos e superar obstáculos calculando ou ajustando as funções matemáticas para determinar a trajetória correta dos disparos e saltos.
---
## 🎯 Objetivos Pedagógicos
- Estimular o raciocínio lógico e a compreensão geométrica de funções.
- Visualizar o comportamento de variáveis e coeficientes em tempo real (ex.: $f(x) = ax + b$ ou $f(x) = ax^2 + bx + c$).
- Tornar o aprendizado de matemática envolvente por meio de mecânicas de *gamificação*.
---
## 🎨 Estética e Visual
- **Estilo**: *Pencil Sketch / Hand-drawn* em papel quadriculado (*graph paper*).
- **Ambiente**: O cenário funciona como uma malha de coordenadas cartesianas $(x, y)$, permitindo que o jogador utilize o próprio grid visual como ferramenta de cálculo para mirar nos alvos.
---
## 🛠️ Tecnologias Utilizadas
- **Linguagem**: C (Padrão C99/C11)
- **Biblioteca Gráfica**: [Allegro 5](https://liballeg.org/)
  - `allegro_primitives` (renderização de formas e traçado das funções em tempo real)
  - `allegro_image` (carregamento de texturas e sprites)
  - `allegro_font` / `allegro_ttf` (textos e equações na tela)
  - `allegro_audio` / `allegro_acodec` (efeitos sonoros e trilha)
- **Ambiente de Desenvolvimento (IDE)**: Microsoft Visual Studio
