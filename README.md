# 💡 SmartLight Webserver com RP2040

Este repositório contém o projeto **SmartLight**, um sistema de iluminação inteligente baseado no microcontrolador **RP2040** e na placa **BitDogLab**. O sistema utiliza uma matriz de LEDs WS2812 controlada por uma interface web acessível via **Wi-Fi**, permitindo ao usuário ajustar iluminação em tempo real.

---

## 📌 Funcionalidades

- ✅ Ligar e desligar a matriz de LEDs RGB
- 🎨 Selecionar modos de cor: Luz Fria, Luz Quente, Luz Amarela e Modo Festa
- 🌈 Efeitos visuais dinâmicos no modo festa
- 🔆 Controle de brilho (0% a 100%)
- 🌐 Interface web embutida no firmware (HTML/CSS)
- 📲 Comunicação via HTTP GET
- ⚙️ Código otimizado para execução no RP2040

---

## 🧠 Arquitetura do Sistema

O projeto é dividido em:

1. **Interface Web**  
   - HTML/CSS gerado diretamente no firmware
   - Feedback visual em tempo real

2. **Servidor HTTP no RP2040**
   - Utiliza a pilha lwIP para conexões Wi-Fi
   - Escuta requisições na porta 80
   - Endpoints como `/toggle`, `/mode?value=X`, `/brightness?value=Y`

3. **Controle dos LEDs**
   - Utiliza o subsistema PIO para controle preciso da matriz WS2812
   - Modos definidos com constantes: `MODE_OFF`, `MODE_COLD`, `MODE_WARM`, `MODE_YELLOW`, `MODE_PARTY`
   - Brilho ajustado via multiplicação dos valores RGB

---

## 📷 Interface

A interface é limpa, responsiva e com feedback visual:
- A cor de fundo muda conforme o modo escolhido
- Controle deslizante de brilho com resposta imediata
- Botões intuitivos para seleção de modos

---

## 🛠️ Requisitos

- Placa **RP2040** com **Wi-Fi CYW43439** (BitDogLab ou similar)
- Matriz de LEDs **WS2812** (5x5 ou compatível)
- Compilador **CMake + Pico SDK**
- Navegador web para acesso à interface

---
