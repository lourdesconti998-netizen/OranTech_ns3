import sqlite3
import xml.etree.ElementTree as ET
import re

# -----------------------------
# CONFIGURACIÓN
# -----------------------------
DB_PATH = "oran-repository.db"
XML_PATH = "oran-monitoring.xml"

# Tablas que queremos exportar
MONITORING_TABLES = [
    "nodeapploss",
    "nodelocation",
    "nruecqi",
    "nruersrprsrq",
    "nruetxbuffer",
    "nruetxdrop",
    "nruetxpdu"
]


# -----------------------------
# FUNCION: limpiar nombres para XML
# -----------------------------
def sanitize_tag(tag: str) -> str:
    tag = str(tag).strip()

    tag = re.sub(r'[^a-zA-Z0-9_:-]', '_', tag)

    if not tag:
        tag = "column"

    if not re.match(r'^[a-zA-Z_]', tag):
        tag = f"col_{tag}"

    return tag


# -----------------------------
# FUNCION: limpiar texto para XML
# -----------------------------
def sanitize_xml_text(value) -> str:

    if value is None:
        return ""

    text = str(value)

    text = re.sub(r'[\x00-\x08\x0B\x0C\x0E-\x1F]', '', text)

    return text


# -----------------------------
# FUNCION: indentación XML
# -----------------------------
def indent_xml(elem, level=0):

    indent = "\n" + level * "  "

    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = indent + "  "

        for child in elem:
            indent_xml(child, level + 1)

        if not elem[-1].tail or not elem[-1].tail.strip():
            elem[-1].tail = indent
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = indent


# -----------------------------
# FUNCION PRINCIPAL
# -----------------------------
def export_sqlite_to_xml(db_path: str, xml_path: str):

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    root = ET.Element("database")

    summary = []

    # ------------------------------------
    # Recorrer solo las tablas seleccionadas
    # ------------------------------------
    for table_name in MONITORING_TABLES:

        print(f"Exportando tabla: {table_name}")

        cursor.execute(f'SELECT * FROM "{table_name}"')

        column_names = [desc[0] for desc in cursor.description]

        table_elem = ET.SubElement(root, "table", name=table_name)

        row_count = 0

        for row_idx, row in enumerate(cursor, start=1):

            row_elem = ET.SubElement(table_elem, "row")

            for col_name, value in zip(column_names, row):

                safe_col_name = sanitize_tag(col_name)

                col_elem = ET.SubElement(row_elem, safe_col_name)

                if value is None:
                    col_elem.set("null", "true")
                else:
                    col_elem.text = sanitize_xml_text(value)

            row_count += 1

        table_elem.set("rows", str(row_count))

        summary.append((table_name, row_count, len(column_names)))

    # ------------------------------------
    # Crear nodo resumen
    # ------------------------------------
    summary_elem = ET.Element("summary")

    for table_name, row_count, col_count in summary:

        ET.SubElement(
            summary_elem,
            "table",
            name=table_name,
            rows=str(row_count),
            columns=str(col_count)
        )

    root.insert(0, summary_elem)

    indent_xml(root)

    tree = ET.ElementTree(root)
    tree.write(xml_path, encoding="utf-8", xml_declaration=True)

    conn.close()

    print("\nXML generado correctamente:", xml_path)

    print("\nResumen de tablas exportadas:")
    for table_name, row_count, col_count in summary:
        print(f"  - {table_name}: {row_count} filas, {col_count} columnas")


# -----------------------------
# EJECUCIÓN DEL SCRIPT
# -----------------------------
if __name__ == "__main__":
    export_sqlite_to_xml(DB_PATH, XML_PATH)