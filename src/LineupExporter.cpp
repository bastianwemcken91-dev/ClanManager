#include "LineupExporter.h"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDebug>

static QString colorToRgbHex(const QColor &c)
{
    QColor cc = c.isValid() ? c : QColor(Qt::white);
    return QString("%1%2%3%4").arg("FF").arg(cc.red(), 2, 16, QChar('0')).arg(cc.green(), 2, 16, QChar('0')).arg(cc.blue(), 2, 16, QChar('0')).toUpper();
}

bool LineupExporter::writeXlsx(const QString &filePath, const QString &commander, const QVector<LineupTrupp> &trupps)
{
    QTemporaryDir tmp;
    if (!tmp.isValid())
        return false;
    QDir base(tmp.path());

    // create necessary directories
    base.mkpath("_rels");
    base.mkpath("xl/_rels");
    base.mkpath("xl/worksheets");

    // 1) [Content_Types].xml
    QFile types(base.filePath("[Content_Types].xml"));
    if (!types.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    {
        QTextStream out(&types);
        out << R"(<?xml version="1.0" encoding="UTF-8"?>")" << "\n";
        out << R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">)" << "\n";
        out << R"(  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>)" << "\n";
        out << R"(  <Default Extension="xml" ContentType="application/xml"/>)" << "\n";
        out << R"(  <Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>)" << "\n";
        out << R"(  <Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>)" << "\n";
        out << R"(  <Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>)" << "\n";
        out << R"(</Types>)" << "\n";
        types.close();
    }

    // 2) _rels/.rels
    QFile rels(base.filePath("_rels/.rels"));
    if (!rels.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    {
        QTextStream out(&rels);
        out << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
        out << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)" << "\n";
        out << R"(  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>)" << "\n";
        out << R"(</Relationships>)" << "\n";
        rels.close();
    }

    // 3) xl/workbook.xml
    QFile wb(base.filePath("xl/workbook.xml"));
    if (!wb.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    {
        QTextStream out(&wb);
        out << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
        out << R"(<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)" << "\n";
        out << R"(  <sheets>)" << "\n";
        out << R"(    <sheet name="Aufstellung" sheetId="1" r:id="rId1"/>)" << "\n";
        out << R"(  </sheets>)" << "\n";
        out << R"(</workbook>)" << "\n";
        wb.close();
    }

    // 4) xl/_rels/workbook.xml.rels
    QFile wbr(base.filePath("xl/_rels/workbook.xml.rels"));
    if (!wbr.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    {
        QTextStream out(&wbr);
        out << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
        out << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)" << "\n";
        out << R"(  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>)" << "\n";
        out << R"(  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>)" << "\n";
        out << R"(</Relationships>)" << "\n";
        wbr.close();
    }

    // styles.xml - minimal, include fills for trupp colors
    QFile styles(base.filePath("xl/styles.xml"));
    if (!styles.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    {
        QTextStream out(&styles);
        out << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
        out << R"(<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">)" << "\n";
        out << R"(  <fonts count="1"><font><sz val="11"/><color rgb="FF000000"/><name val="Calibri"/></font></fonts>)" << "\n";

        // base fills (required: two fills)
        out << R"(  <fills count=")" << QString::number(2 + trupps.size()) << R"(">)" << "\n";
        out << R"(    <fill><patternFill patternType="none"/></fill>)" << "\n";
        out << R"(    <fill><patternFill patternType="gray125"/></fill>)" << "\n";
        // additional solid fills for each trupp color
        for (const LineupTrupp &t : trupps)
        {
            QString rgb = colorToRgbHex(t.color);
            out << R"(    <fill>)" << "\n";
            out << R"(      <patternFill patternType="solid">)" << "\n";
            out << R"(        <fgColor rgb=")" << rgb << R"("/><bgColor indexed="64"/>)" << "\n";
            out << R"(      </patternFill>)" << "\n";
            out << R"(    </fill>)" << "\n";
        }
        out << R"(  </fills>)" << "\n";

        // borders count 1 (none)
        out << R"(  <borders count="1"><border><left/><right/><top/><bottom/><diagonal/></border></borders>)" << "\n";

        // cellStyleXfs
        out << R"(  <cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>)" << "\n";

        // cellXfs - one per fill (style index equals order here)
        int xfCount = 1 + (int)trupps.size();
        out << "  <cellXfs count=\"" << QString::number(xfCount) << "\">\n";
        // default xf (no fill)
        out << R"(    <xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/>)" << "\n";
        // subsequent xf entries map to our trupp fills: fillId starts at 2 (0 and 1 used by defaults)
        for (int i = 0; i < trupps.size(); ++i)
        {
            int fillId = 2 + i; // as we added two base fills
            out << "    <xf numFmtId=\"0\" fontId=\"0\" fillId=\"" << QString::number(fillId) << "\" borderId=\"0\" xfId=\"0\" applyFill=\"1\"/>\n";
        }
        out << "  </cellXfs>\n";

        // cellStyles placeholder
        out << R"(  <cellStyles count="1"><cellStyle name="Normal" xfId="0" builtinId="0"/></cellStyles>)" << "\n";
        out << R"(  <dxfs count="0"/>)" << "\n";
        out << R"(  <tableStyles count="0" defaultTableStyle="" defaultPivotStyle=""/>)" << "\n";
        out << R"(</styleSheet>)" << "\n";
        styles.close();
    }

    // Build sheet XML
    QFile sheet(base.filePath("xl/worksheets/sheet1.xml"));
    if (!sheet.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    {
        QTextStream out(&sheet);
        out << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
        out << R"(<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">)" << "\n";
        out << R"(  <sheetData>)" << "\n";

        int currentRow = 1;
        // Commander row at A1
        out << QString("    <row r=\"%1\">\n").arg(currentRow);
        out << QString("      <c r=\"A%1\" t=\"inlineStr\">\n").arg(currentRow);
        out << QString("        <is><t>Kommandant: %1</t></is>\n").arg(commander.toHtmlEscaped());
        out << "      </c>\n";
        out << "    </row>\n";

        currentRow += 1;
        // empty spacer row
        currentRow += 1;

        // header row for trupps (one trupp per column)
        int headerRow = currentRow;
        out << QString("    <row r=\"%1\">\n").arg(headerRow);
        for (int col = 0; col < trupps.size(); ++col)
        {
            // compute Excel column letter
            int c = col;
            QString colLetter;
            while (true)
            {
                colLetter.prepend(QChar('A' + (c % 26)));
                c = c / 26 - 1;
                if (c < 0)
                    break;
            }
            // style index: 1 + col (since xf 0 is default)
            int sidx = 1 + col;
            out << QString("      <c r=\"%1%2\" t=\"inlineStr\" s=\"%3\">\n").arg(colLetter).arg(headerRow).arg(sidx);
            out << QString("        <is><t>%1</t></is>\n").arg(trupps[col].name.toHtmlEscaped());
            out << "      </c>\n";
        }
        out << "    </row>\n";

        // compute max players in any trupp
        int maxPlayers = 0;
        for (const LineupTrupp &t : trupps)
            maxPlayers = qMax(maxPlayers, t.players.size());

        // write player rows
        for (int i = 0; i < maxPlayers; ++i)
        {
            int r = headerRow + 1 + i;
            out << QString("    <row r=\"%1\">\n").arg(r);
            for (int col = 0; col < trupps.size(); ++col)
            {
                int c = col;
                QString colLetter;
                while (true)
                {
                    colLetter.prepend(QChar('A' + (c % 26)));
                    c = c / 26 - 1;
                    if (c < 0)
                        break;
                }
                QString text;
                if (i < trupps[col].players.size())
                    text = trupps[col].players.at(i).toHtmlEscaped();
                else
                    text = "";
                out << QString("      <c r=\"%1%2\" t=\"inlineStr\">\n").arg(colLetter).arg(r);
                out << QString("        <is><t>%1</t></is>\n").arg(text);
                out << "      </c>\n";
            }
            out << "    </row>\n";
        }

        out << "  </sheetData>\n";
        out << "</worksheet>\n";
        sheet.close();
    }

    // Zip all files into the target .xlsx using system zip command
    // Use -r (recursive) and -q (quiet). Ensure target directory exists.
    QFileInfo fi(filePath);
    QDir targetDir = fi.dir();
    if (!targetDir.exists())
        targetDir.mkpath(".");

    // use QProcess to run zip
    QProcess p;
    QStringList args;
    args << "-r" << "-q" << filePath << ".";
    p.setWorkingDirectory(tmp.path());
    p.start("zip", args);
    bool ok = p.waitForFinished(60000); // wait up to 60s
    if (!ok)
    {
        qWarning() << "zip process failed or timed out";
        return false;
    }
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0)
    {
        qWarning() << "zip exit code" << p.exitCode() << p.readAllStandardError();
        return false;
    }

    return true;
}
