from pathlib import Path
from textwrap import wrap


OUT = Path(r"C:\Users\lucas\OneDrive\Documentos\New project\Projeto_Monitoramento_Gado.pdf")

PAGE_W = 595
PAGE_H = 842
MARGIN = 48


def esc_text(text):
    data = str(text).encode("cp1252", errors="replace")
    out = []
    for b in data:
        if b in (40, 41, 92):
            out.append("\\" + chr(b))
        elif b < 32 or b > 126:
            out.append("\\" + format(b, "03o"))
        else:
            out.append(chr(b))
    return "".join(out)


class Page:
    def __init__(self):
        self.ops = []

    def text(self, x, y, txt, size=10, font="F1", color=(0, 0, 0)):
        r, g, b = color
        self.ops.append(f"{r:.3f} {g:.3f} {b:.3f} rg")
        self.ops.append(f"BT /{font} {size} Tf {x:.1f} {y:.1f} Td ({esc_text(txt)}) Tj ET")

    def line(self, x1, y1, x2, y2, color=(0, 0, 0), width=1):
        r, g, b = color
        self.ops.append(f"{r:.3f} {g:.3f} {b:.3f} RG {width:.1f} w")
        self.ops.append(f"{x1:.1f} {y1:.1f} m {x2:.1f} {y2:.1f} l S")

    def rect(self, x, y, w, h, stroke=(0, 0, 0), fill=None, width=1):
        if fill is not None:
            r, g, b = fill
            self.ops.append(f"{r:.3f} {g:.3f} {b:.3f} rg")
            self.ops.append(f"{x:.1f} {y:.1f} {w:.1f} {h:.1f} re f")
        r, g, b = stroke
        self.ops.append(f"{r:.3f} {g:.3f} {b:.3f} RG {width:.1f} w")
        self.ops.append(f"{x:.1f} {y:.1f} {w:.1f} {h:.1f} re S")

    def circle(self, x, y, r, stroke=(0, 0, 0), fill=None, width=1):
        # Bezier circle approximation.
        c = 0.55228475 * r
        if fill is not None:
            fr, fg, fb = fill
            self.ops.append(f"{fr:.3f} {fg:.3f} {fb:.3f} rg")
            paint = "B"
        else:
            paint = "S"
        sr, sg, sb = stroke
        self.ops.append(f"{sr:.3f} {sg:.3f} {sb:.3f} RG {width:.1f} w")
        self.ops.append(
            f"{x+r:.1f} {y:.1f} m "
            f"{x+r:.1f} {y+c:.1f} {x+c:.1f} {y+r:.1f} {x:.1f} {y+r:.1f} c "
            f"{x-c:.1f} {y+r:.1f} {x-r:.1f} {y+c:.1f} {x-r:.1f} {y:.1f} c "
            f"{x-r:.1f} {y-c:.1f} {x-c:.1f} {y-r:.1f} {x:.1f} {y-r:.1f} c "
            f"{x+c:.1f} {y-r:.1f} {x+r:.1f} {y-c:.1f} {x+r:.1f} {y:.1f} c {paint}"
        )

    def stream(self):
        return "\n".join(self.ops).encode("cp1252", errors="replace")


class Doc:
    def __init__(self):
        self.pages = []

    def page(self):
        p = Page()
        self.pages.append(p)
        return p

    def save(self, path):
        objects = []

        def add(obj):
            objects.append(obj)
            return len(objects)

        font_regular = add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>")
        font_bold = add(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>")

        page_ids = []
        content_ids = []
        for p in self.pages:
            data = p.stream()
            content_ids.append(add(b"<< /Length " + str(len(data)).encode() + b" >>\nstream\n" + data + b"\nendstream"))
            page_ids.append(None)

        pages_id_placeholder = len(objects) + len(self.pages) + 1
        for i, _ in enumerate(self.pages):
            page_ids[i] = add(
                f"<< /Type /Page /Parent {pages_id_placeholder} 0 R /MediaBox [0 0 {PAGE_W} {PAGE_H}] "
                f"/Resources << /Font << /F1 {font_regular} 0 R /F2 {font_bold} 0 R >> >> "
                f"/Contents {content_ids[i]} 0 R >>".encode()
            )

        kids = " ".join(f"{pid} 0 R" for pid in page_ids)
        pages_id = add(f"<< /Type /Pages /Kids [{kids}] /Count {len(page_ids)} >>".encode())
        catalog_id = add(f"<< /Type /Catalog /Pages {pages_id} 0 R >>".encode())

        assert pages_id == pages_id_placeholder

        out = [b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n"]
        xref = [0]
        pos = len(out[0])
        for i, obj in enumerate(objects, start=1):
            chunk = f"{i} 0 obj\n".encode() + obj + b"\nendobj\n"
            xref.append(pos)
            out.append(chunk)
            pos += len(chunk)
        xref_pos = pos
        out.append(f"xref\n0 {len(objects)+1}\n".encode())
        out.append(b"0000000000 65535 f \n")
        for off in xref[1:]:
            out.append(f"{off:010d} 00000 n \n".encode())
        out.append(f"trailer\n<< /Size {len(objects)+1} /Root {catalog_id} 0 R >>\nstartxref\n{xref_pos}\n%%EOF\n".encode())
        path.write_bytes(b"".join(out))


class Writer:
    def __init__(self, doc):
        self.doc = doc
        self.p = None
        self.y = 0

    def new_page(self, title=None):
        self.p = self.doc.page()
        self.y = PAGE_H - MARGIN
        if title:
            self.p.text(MARGIN, self.y, title, 18, "F2", (0.08, 0.18, 0.28))
            self.y -= 22
            self.p.line(MARGIN, self.y, PAGE_W - MARGIN, self.y, (0.65, 0.72, 0.78), 0.8)
            self.y -= 20

    def heading(self, txt):
        self.ensure(40)
        self.y -= 6
        self.p.text(MARGIN, self.y, txt, 13, "F2", (0.08, 0.24, 0.16))
        self.y -= 18

    def para(self, txt, size=10, leading=13, width=88):
        for line in wrap(txt, width=width):
            self.ensure(leading + 4)
            self.p.text(MARGIN, self.y, line, size)
            self.y -= leading
        self.y -= 5

    def bullet(self, txt):
        for idx, line in enumerate(wrap(txt, width=83)):
            self.ensure(16)
            prefix = "- " if idx == 0 else "  "
            self.p.text(MARGIN, self.y, prefix + line, 10)
            self.y -= 13

    def table(self, headers, rows, col_widths, size=8.5):
        total_w = sum(col_widths)
        row_h = 18
        self.ensure(row_h * (len(rows) + 2))
        x = MARGIN
        self.p.rect(x, self.y - row_h + 4, total_w, row_h, stroke=(0.70, 0.74, 0.78), fill=(0.90, 0.94, 0.92))
        cx = x
        for h, w in zip(headers, col_widths):
            self.p.text(cx + 4, self.y - 8, h, size, "F2")
            self.p.line(cx, self.y + 4, cx, self.y - row_h + 4, (0.70, 0.74, 0.78), 0.5)
            cx += w
        self.p.line(x + total_w, self.y + 4, x + total_w, self.y - row_h + 4, (0.70, 0.74, 0.78), 0.5)
        self.y -= row_h

        for row in rows:
            max_lines = max(len(wrap(str(cell), width=max(10, int(w / 5.2)))) for cell, w in zip(row, col_widths))
            h = max(row_h, 10 + max_lines * 10)
            self.ensure(h + 4)
            self.p.rect(x, self.y - h + 4, total_w, h, stroke=(0.82, 0.84, 0.86), fill=None, width=0.5)
            cx = x
            for cell, w in zip(row, col_widths):
                lines = wrap(str(cell), width=max(10, int(w / 5.2)))
                ty = self.y - 8
                for line in lines:
                    self.p.text(cx + 4, ty, line, size)
                    ty -= 10
                self.p.line(cx, self.y + 4, cx, self.y - h + 4, (0.82, 0.84, 0.86), 0.5)
                cx += w
            self.p.line(x + total_w, self.y + 4, x + total_w, self.y - h + 4, (0.82, 0.84, 0.86), 0.5)
            self.y -= h
        self.y -= 8

    def ensure(self, needed):
        if self.y - needed < MARGIN:
            self.new_page("Projeto: Monitoramento de Gado")


def draw_schematic(p):
    def box(x, y, w, h, title, lines, fill):
        p.rect(x, y, w, h, stroke=(0.25, 0.32, 0.36), fill=fill, width=1)
        p.text(x + 8, y + h - 16, title, 11, "F2")
        ty = y + h - 31
        for line in lines:
            p.text(x + 8, ty, line, 8.2)
            ty -= 11

    def connect(x1, y1, x2, y2, label):
        p.line(x1, y1, x2, y2, (0.20, 0.38, 0.48), 1)
        mx = (x1 + x2) / 2
        my = (y1 + y2) / 2
        p.text(mx - 28, my + 4, label, 7.5, "F1", (0.05, 0.25, 0.35))

    box(218, 330, 160, 170, "Arduino Mega 2560", [
        "Central do projeto",
        "Le sensores",
        "Mostra no LCD",
        "Grava DADOS.CSV no SD",
        "Serial Monitor: 9600 baud",
    ], (0.92, 0.96, 1.00))

    box(52, 640, 160, 92, "Barramento I2C", [
        "LCD 16x2 I2C",
        "RTC DS3231",
        "BME280",
        "SDA pino 20 / SCL pino 21",
    ], (0.93, 0.98, 0.93))
    connect(212, 670, 218, 470, "I2C")

    box(388, 640, 155, 92, "Barramento SPI", [
        "RFID RC522",
        "Modulo SD",
        "MISO 50 / MOSI 51",
        "SCK 52 / CS separados",
    ], (0.98, 0.95, 0.90))
    connect(388, 670, 378, 470, "SPI")

    box(45, 410, 150, 82, "Sensores digitais", [
        "PIR OUT -> pino 6",
        "HC-SR04 Trig -> 9",
        "HC-SR04 Echo -> 10",
    ], (0.98, 0.94, 0.94))
    connect(195, 450, 218, 425, "Digital")

    box(410, 410, 125, 82, "GPS NEO-6M", [
        "TX -> RX1 pino 19",
        "GND comum",
        "VCC conforme modulo",
        "RX pode ficar solto",
    ], (0.94, 0.94, 0.99))
    connect(410, 445, 378, 425, "Serial1")

    box(210, 210, 175, 70, "Alimentacao e GND", [
        "Todos os GNDs em comum",
        "RC522 e BME280 em 3.3V",
        "LCD/RTC/PIR/SD/HC-SR04 em 5V",
    ], (0.96, 0.96, 0.96))
    connect(298, 330, 298, 280, "5V/3.3V/GND")

    p.text(52, 175, "Observacao: este esquema e simplificado. Ele mostra os barramentos e pinos principais; confira a tabela de ligacoes para cada fio.", 9)


def main():
    doc = Doc()
    w = Writer(doc)

    w.new_page("Projeto: Monitoramento de Gado")
    w.para("Sistema montado com Arduino Mega 2560 para acompanhar condicoes de um ambiente rural ou maquete de manejo de gado. O projeto mede temperatura, umidade, pressao, movimento, distancia/nivel, identifica tags RFID, registra data e hora e salva tudo em um arquivo CSV no cartao SD.")

    w.heading("Objetivo")
    for item in [
        "Monitorar dados ambientais e eventos de passagem/movimento.",
        "Registrar as informacoes localmente no cartao SD, sem depender de internet.",
        "Mostrar leituras principais no LCD 16x2 para conferencia rapida.",
        "Usar GPS direto no Arduino Mega para registrar localizacao quando houver sinal de satelite.",
    ]:
        w.bullet(item)

    w.heading("Componentes usados")
    rows = [
        ("Arduino Mega 2560", "Controlador principal do projeto."),
        ("LCD 16x2 I2C", "Mostra temperatura, umidade, pressao, nivel, movimento, SD e GPS."),
        ("RTC DS3231", "Mantem data e hora corretas para o arquivo CSV."),
        ("BME280", "Mede temperatura do ar, umidade e pressao atmosferica."),
        ("PIR", "Detecta movimento/presenca."),
        ("HC-SR04", "Mede distancia, usado como referencia de nivel."),
        ("RC522 RFID", "Le tags/cartoes RFID para registrar identificacao."),
        ("Modulo SD", "Salva o arquivo DADOS.CSV."),
        ("GPS NEO-6M", "Fornece latitude, longitude e numero de satelites."),
    ]
    w.table(("Componente", "Funcao"), rows, (165, 330), 9)

    w.heading("Como funciona")
    w.para("A cada 5 segundos o Mega le os sensores, atualiza as variaveis internas, imprime uma linha no Serial Monitor e grava a mesma linha no cartao SD. O LCD troca de tela a cada 2,5 segundos para mostrar informacoes diferentes. Quando uma tag RFID e aproximada, o codigo registra um evento RFID separado no CSV.")
    w.para("O GPS e lido continuamente pela porta Serial1 do Mega. Mesmo antes de fixar satelites, o codigo mostra quantos caracteres estao chegando do GPS. Se GPS chars aumenta, a ligacao esta correta; se Sat fica 0, falta apenas sinal de satelite.")

    w.new_page("Ligacoes Eletronicas")
    w.heading("Tabela de ligacoes")
    rows = [
        ("LCD I2C", "VCC, GND, SDA, SCL", "5V, GND, pino 20, pino 21"),
        ("RTC DS3231", "VCC, GND, SDA, SCL", "5V, GND, pino 20, pino 21"),
        ("BME280", "VCC, GND, SDA, SCL", "3.3V, GND, pino 20, pino 21"),
        ("RFID RC522", "VCC, GND, RST, SDA/SS, MOSI, MISO, SCK", "3.3V, GND, pino 5, pino 53, 51, 50, 52"),
        ("Modulo SD", "VCC, GND, MISO, MOSI, SCK, CS", "5V, GND, pino 50, 51, 52, 48"),
        ("PIR", "VCC, OUT, GND", "5V, pino 6, GND"),
        ("HC-SR04", "VCC, Trig, Echo, GND", "5V, pino 9, pino 10, GND"),
        ("GPS NEO-6M", "VCC, GND, TX, RX", "5V ou 3.3V conforme modulo, GND, TX no pino 19, RX desligado"),
    ]
    w.table(("Modulo", "Pinos do modulo", "Ligacao no Mega"), rows, (105, 175, 215), 8.2)

    w.heading("Cuidados importantes")
    for item in [
        "Todos os GNDs precisam estar interligados. Sem GND comum, os sensores podem falhar ou dar leituras falsas.",
        "RC522 e BME280 devem ser alimentados em 3.3V. Nao ligue estes modulos em 5V se eles nao tiverem regulador proprio.",
        "No Arduino Mega, I2C usa SDA no pino 20 e SCL no pino 21. Nao use A4/A5, pois isso e do Arduino Uno.",
        "O SD precisa estar em FAT32. Cartoes de 4 GB ou 8 GB costumam funcionar melhor.",
        "O GPS precisa ficar em area aberta ou perto de janela. Dentro de casa ele pode receber caracteres mas ficar com Sat: 0.",
    ]:
        w.bullet(item)

    w.new_page("Esquema Eletronico Simplificado")
    draw_schematic(w.p)

    w.new_page("Funcionamento do Codigo")
    w.heading("Bibliotecas")
    for item in [
        "Wire e SPI: comunicacao I2C e SPI.",
        "LiquidCrystal_I2C: controle do LCD 16x2.",
        "RTClib: leitura do RTC DS3231.",
        "MFRC522: leitura do RFID RC522.",
        "Adafruit_BME280 e Adafruit Unified Sensor: leitura do BME280.",
        "SD: gravacao do arquivo DADOS.CSV.",
        "TinyGPSPlus: interpretacao das frases NMEA do GPS.",
    ]:
        w.bullet(item)

    w.heading("Leituras e filtros")
    w.para("O PIR tem 60 segundos de aquecimento. Durante esse tempo o Serial mostra Movimento: AQUECENDO e o CSV grava movimento como 0. Depois disso, o codigo confirma varias leituras antes de aceitar movimento. O campo PIR raw no Serial mostra o sinal fisico que chega ao pino 6: HIGH significa que o sensor esta acionado; LOW significa sem deteccao.")
    w.para("O HC-SR04 faz tres medicoes e usa uma leitura mais estavel para diminuir pulos de distancia. Se nenhuma leitura voltar dentro do tempo limite, a distancia aparece como NA.")
    w.para("O BME280 tenta automaticamente os enderecos 0x76 e 0x77, porque diferentes modulos podem vir configurados com enderecos diferentes.")

    w.heading("Arquivo CSV")
    rows = [
        ("data", "Data do RTC no formato AAAA-MM-DD."),
        ("hora", "Hora do RTC no formato HH:MM:SS."),
        ("temp_ar_c", "Temperatura do ar em graus Celsius."),
        ("umidade_pct", "Umidade relativa do ar em porcentagem."),
        ("pressao_hpa", "Pressao atmosferica em hPa."),
        ("distancia_cm", "Distancia medida pelo HC-SR04 em centimetros."),
        ("movimento", "1 quando ha movimento confirmado, 0 quando nao ha."),
        ("gps_valido", "1 quando latitude/longitude estao validas, 0 quando nao."),
        ("latitude/longitude", "Coordenadas do GPS ou NA quando ainda sem sinal."),
        ("gps_satelites", "Quantidade de satelites informada pelo GPS."),
        ("evento", "LEITURA para leitura comum ou RFID quando uma tag e lida."),
        ("rfid_uid", "Codigo da tag RFID, quando houver."),
    ]
    w.table(("Campo", "Descricao"), rows, (150, 345), 8.2)

    w.new_page("Teste e Solucao de Problemas")
    w.heading("Ordem de teste recomendada")
    for item in [
        "1. LCD acende e mostra Monitoramento / Inicializando.",
        "2. Serial Monitor em 9600 baud mostra RTC: OK, BME280: OK e SD: OK.",
        "3. BME280 mostra temperatura, umidade e pressao.",
        "4. HC-SR04 muda distancia quando voce aproxima ou afasta uma superficie.",
        "5. PIR fica AQUECENDO no primeiro minuto e depois deve ficar NAO quando parado.",
        "6. GPS chars deve aumentar. Se aumentar, o GPS esta ligado certo; espere satelites.",
        "7. Aproxime uma tag RFID e confira se aparece RFID detectado.",
        "8. Remova o cartao SD e abra DADOS.CSV no computador.",
    ]:
        w.bullet(item)

    w.heading("Mensagens importantes")
    rows = [
        ("GPS chars: 0", "Mega nao esta recebendo o GPS. Confira TX do GPS no pino 19 e GND."),
        ("GPS chars aumentando e Sat: 0", "Ligacao do GPS esta certa, mas ainda nao ha sinal de satelite."),
        ("PIR raw: HIGH direto", "O PIR esta mandando movimento o tempo todo. Diminua sensibilidade/tempo ou reposicione."),
        ("Distancia: NA", "O HC-SR04 nao recebeu eco. Confira Trig 9, Echo 10 e posicao do sensor."),
        ("SD: ERRO", "Cartao nao inicializou. Confira FAT32, CS 48 e pinos SPI 50/51/52."),
        ("BME280: ERRO", "Confira VCC em 3.3V, SDA 20, SCL 21 e GND."),
    ]
    w.table(("Mensagem", "O que significa"), rows, (170, 325), 8.5)

    w.heading("Resumo para apresentacao")
    w.para("Este projeto simula um sistema de monitoramento para gado. Ele coleta dados ambientais, movimento, distancia de nivel e identificacao RFID, registra data/hora e salva tudo em cartao SD. O GPS direto no Mega permite associar as leituras a uma localizacao quando houver sinal. A solucao funciona sem internet, o que e util em locais rurais onde a conexao pode ser instavel.")

    doc.save(OUT)
    print(OUT)


if __name__ == "__main__":
    main()
