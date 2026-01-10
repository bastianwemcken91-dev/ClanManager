#!/usr/bin/env bash
set -euo pipefail

# Security & SBOM Scan Script for macOS (zsh/bash)
# - Installiert/prüft Trivy, cdxgen und Microsoft SBOM Tool
# - Führt SBOM-Scans und Sicherheitsanalysen aus
# - Speichert Ergebnisse als JSON/HTML auf dem Desktop

have() { command -v "$1" >/dev/null 2>&1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${1:-$PWD}"
SCAN_TYPE="${2:-all}"  # all, sbom, vuln, secret, config, license
DESKTOP="${HOME}/Desktop"

# Output-Dateien
TRIVY_SBOM_OUT="$DESKTOP/trivy-sbom.json"
TRIVY_VULN_OUT="$DESKTOP/trivy-vulnerabilities.json"
TRIVY_SECRET_OUT="$DESKTOP/trivy-secrets.json"
TRIVY_CONFIG_OUT="$DESKTOP/trivy-misconfig.json"
TRIVY_LICENSE_OUT="$DESKTOP/trivy-licenses.json"
TRIVY_REPORT_HTML="$DESKTOP/trivy-report.html"

CDX_OUT="$DESKTOP/cdxgen-sbom.json"
MS_OUT="$DESKTOP/microsoft-sbom.json"

# Kompatibilität
TRIVY_OUT="$TRIVY_SBOM_OUT"
TRIVY_OUTPUT="$TRIVY_SBOM_OUT"
CDX_OUTPUT="$CDX_OUT"
MS_OUTPUT="$MS_OUT"

INSTALL_BIN="$HOME/.local/bin"

mkdir -p "$DESKTOP" "$INSTALL_BIN"
export PATH="$INSTALL_BIN:$PATH"

usage() {
  cat <<EOF
Verwendung: $(basename "$0") [PROJEKT_PFAD] [SCAN_TYP]

Argumente:
  PROJEKT_PFAD    Ordner, der gescannt werden soll (Standard: aktuelles Verzeichnis)
  SCAN_TYP        Art des Scans (Standard: all)
                  - all:     Alle Scans (SBOM + Security)
                  - sbom:    Nur SBOM-Generierung (Trivy, cdxgen, Microsoft)
                  - vuln:    Nur Vulnerability-Scan mit Trivy
                  - secret:  Nur Secret-Detection mit Trivy
                  - config:  Nur Misconfiguration-Scan mit Trivy
                  - license: Nur License-Scan mit Trivy

Trivy-Features:
  - SBOM-Generierung (CycloneDX, SPDX)
  - Vulnerability-Scanning (CVE-Datenbank)
  - Secret-Detection (API-Keys, Passwörter, Tokens)
  - Misconfiguration-Detection (IaC, Dockerfiles, Kubernetes)
  - License-Compliance-Check

Output-Dateien:
  - ~/Desktop/trivy-sbom.json
  - ~/Desktop/trivy-vulnerabilities.json
  - ~/Desktop/trivy-secrets.json
  - ~/Desktop/trivy-misconfig.json
  - ~/Desktop/trivy-licenses.json
  - ~/Desktop/trivy-report.html (Komplett-Report)
  - ~/Desktop/cdxgen-sbom.json
  - ~/Desktop/microsoft-sbom.json

Beispiele:
  $(basename "$0")                    # Alle Scans im aktuellen Verzeichnis
  $(basename "$0") . vuln             # Nur Vulnerability-Scan
  $(basename "$0") /path/to/project   # Alle Scans in anderem Projekt
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

install_trivy() {
  if have trivy; then
    echo "Trivy bereits installiert: $(command -v trivy)"
    return 0
  fi
  echo "Installiere Trivy..."
  if have brew; then
    brew install trivy || true
  fi
  if ! have trivy; then
    # Fallback: offizielles Install-Skript nach $HOME/.local/bin
    echo "Installiere Trivy via Install-Skript in $INSTALL_BIN ..."
    tmpdir="$(mktemp -d)"; trap 'rm -rf "$tmpdir"' EXIT
    curl -sSfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh -o "$tmpdir/install.sh"
    chmod +x "$tmpdir/install.sh"
    "$tmpdir/install.sh" -b "$INSTALL_BIN" >/dev/null
  fi
  if ! have trivy; then
    echo "Fehler: Trivy konnte nicht installiert werden." >&2
    exit 1
  fi
}

install_cdxgen() {
  if have cdxgen; then
    echo "cdxgen bereits installiert: $(command -v cdxgen)"
    return 0
  fi
  echo "Installiere cdxgen..."
  # Versuche Homebrew (falls verfügbar; Paket evtl. nicht vorhanden)
  if have brew && brew info cdxgen >/dev/null 2>&1; then
    brew install cdxgen || true
  fi
  if have cdxgen; then return 0; fi

  # Node.js für npm/npx sicherstellen
  if ! have node; then
    if have brew; then
      echo "Installiere Node.js via Homebrew..."
      brew install node || true
    fi
  fi

  # Global mit npm installieren (ohne Root, wenn möglich)
  if have npm; then
    npm install -g --location=global @cyclonedx/cdxgen || npm install -g @cyclonedx/cdxgen || true
  fi

  if have cdxgen; then
    echo "cdxgen installiert: $(command -v cdxgen)"
  else
    echo "Hinweis: cdxgen wird zur Laufzeit via npx verwendet."
  fi
}

run_trivy_sbom() {
    echo "🔍 Starte Trivy SBOM Scan in: ${PROJECT_DIR}"
    if trivy fs --format cyclonedx --output "$TRIVY_SBOM_OUT" --scanners license "$PROJECT_DIR" 2>/dev/null; then
        echo "✅ Trivy SBOM: $TRIVY_SBOM_OUT"
    else
        echo "⚠️ Trivy SBOM fehlgeschlagen"
        return 1
    fi
}

run_trivy_vulnerabilities() {
    echo "🔍 Starte Trivy Vulnerability Scan in: ${PROJECT_DIR}"
    if trivy fs --format json --output "$TRIVY_VULN_OUT" --scanners vuln "$PROJECT_DIR" 2>/dev/null; then
        echo "✅ Trivy Vulnerabilities: $TRIVY_VULN_OUT"
        # Zusammenfassung anzeigen
        local critical=$(jq '[.Results[]?.Vulnerabilities[]? | select(.Severity=="CRITICAL")] | length' "$TRIVY_VULN_OUT" 2>/dev/null || echo "0")
        local high=$(jq '[.Results[]?.Vulnerabilities[]? | select(.Severity=="HIGH")] | length' "$TRIVY_VULN_OUT" 2>/dev/null || echo "0")
        local medium=$(jq '[.Results[]?.Vulnerabilities[]? | select(.Severity=="MEDIUM")] | length' "$TRIVY_VULN_OUT" 2>/dev/null || echo "0")
        echo "   → CRITICAL: $critical, HIGH: $high, MEDIUM: $medium"
    else
        echo "⚠️ Trivy Vulnerability-Scan fehlgeschlagen"
        return 1
    fi
}

run_trivy_secrets() {
    echo "🔍 Starte Trivy Secret Detection in: ${PROJECT_DIR}"
    if trivy fs --format json --output "$TRIVY_SECRET_OUT" --scanners secret "$PROJECT_DIR" 2>/dev/null; then
        echo "✅ Trivy Secrets: $TRIVY_SECRET_OUT"
        local secrets=$(jq '[.Results[]?.Secrets[]?] | length' "$TRIVY_SECRET_OUT" 2>/dev/null || echo "0")
        echo "   → Gefundene Secrets: $secrets"
    else
        echo "⚠️ Trivy Secret-Scan fehlgeschlagen"
        return 1
    fi
}

run_trivy_misconfig() {
    echo "🔍 Starte Trivy Misconfiguration Scan in: ${PROJECT_DIR}"
    if trivy fs --format json --output "$TRIVY_CONFIG_OUT" --scanners misconfig "$PROJECT_DIR" 2>/dev/null; then
        echo "✅ Trivy Misconfigurations: $TRIVY_CONFIG_OUT"
        local misconfigs=$(jq '[.Results[]?.Misconfigurations[]?] | length' "$TRIVY_CONFIG_OUT" 2>/dev/null || echo "0")
        echo "   → Gefundene Misconfigurations: $misconfigs"
    else
        echo "⚠️ Trivy Misconfiguration-Scan fehlgeschlagen"
        return 1
    fi
}

run_trivy_licenses() {
    echo "🔍 Starte Trivy License Scan in: ${PROJECT_DIR}"
    if trivy fs --format json --output "$TRIVY_LICENSE_OUT" --scanners license "$PROJECT_DIR" 2>/dev/null; then
        echo "✅ Trivy Licenses: $TRIVY_LICENSE_OUT"
        local licenses=$(jq '[.Results[]?.Licenses[]?] | length' "$TRIVY_LICENSE_OUT" 2>/dev/null || echo "0")
        echo "   → Gefundene Lizenzen: $licenses"
    else
        echo "⚠️ Trivy License-Scan fehlgeschlagen"
        return 1
    fi
}

run_trivy_full_report() {
    echo "🔍 Erstelle Trivy Komplett-Report (HTML) in: ${PROJECT_DIR}"
    if trivy fs --format template --template "@contrib/html.tpl" --output "$TRIVY_REPORT_HTML" "$PROJECT_DIR" 2>/dev/null; then
        echo "✅ Trivy HTML-Report: $TRIVY_REPORT_HTML"
    else
        echo "⚠️ Trivy HTML-Report fehlgeschlagen"
        return 1
    fi
}

run_cdxgen() {
  echo "🔍 Starte cdxgen CycloneDX SBOM Scan in: $PROJECT_DIR"
  
  # Erweiterte cdxgen-Optionen für bessere SBOM-Qualität
  local cdxgen_opts=(
    -o "$CDX_OUT"
    -r                          # Recursive scan
    -t c++                      # Project type: C++
    --spec-version 1.6          # CycloneDX 1.6 spec
    --include-formulation       # Include build/deployment info
    --include-crypto            # Include cryptographic assets
    --author "ClanManager Team"
    --no-bom-validation        # Skip validation for speed
    "$PROJECT_DIR"
  )
  
  if have cdxgen; then
    if cdxgen "${cdxgen_opts[@]}" 2>&1 | grep -v "Atom requires Java"; then
      echo "✅ cdxgen CycloneDX SBOM: $CDX_OUT"
      
      # Statistiken anzeigen (falls jq verfügbar)
      if have jq; then
        local components=$(jq '.components | length' "$CDX_OUT" 2>/dev/null || echo "0")
        local deps=$(jq '.dependencies | length' "$CDX_OUT" 2>/dev/null || echo "0")
        local services=$(jq '.services | length' "$CDX_OUT" 2>/dev/null || echo "0")
        echo "   → Components: $components, Dependencies: $deps, Services: $services"
        
        # CycloneDX Spec Version
        local spec=$(jq -r '.specVersion' "$CDX_OUT" 2>/dev/null || echo "unknown")
        echo "   → CycloneDX Spec: $spec"
      fi
    else
      echo "⚠️ cdxgen mit Warnungen abgeschlossen"
    fi
  else
    if have npx; then
      echo "Nutze npx für cdxgen..."
      npx -y @cyclonedx/cdxgen "${cdxgen_opts[@]}" 2>&1 | grep -v "Atom requires Java" || true
      echo "✅ cdxgen CycloneDX SBOM: $CDX_OUT"
    else
      echo "Fehler: Weder cdxgen noch npx verfügbar. Bitte Node.js/NPM installieren." >&2
      return 1
    fi
  fi
}

install_ms_sbom() {
  if have sbom-tool; then
    echo "Microsoft SBOM Tool bereits installiert: $(command -v sbom-tool)"
    return 0
  fi
  echo "Installiere Microsoft SBOM Tool..."
  
  # Aktuellste Version via GitHub API ermitteln oder feste neuere Version nutzen
  local version="2.2.7"
  local arch="$(uname -m)"
  local binary_name="sbom-tool-osx-x64"
  
  # Für ARM64 (Apple Silicon) könnte es eine andere Variante geben
  if [[ "$arch" == "arm64" ]]; then
    binary_name="sbom-tool-osx-arm64"
  fi
  
  local url="https://github.com/microsoft/sbom-tool/releases/download/v${version}/${binary_name}"
  local target="$INSTALL_BIN/sbom-tool"
  
  # Fallback auf x64, falls arm64 nicht existiert
  if ! curl -sSfL "$url" -o "$target" 2>/dev/null; then
    echo "Versuche x64-Version..."
    url="https://github.com/microsoft/sbom-tool/releases/download/v${version}/sbom-tool-osx-x64"
    if ! curl -sSfL "$url" -o "$target"; then
      echo "⚠️ Microsoft SBOM Tool konnte nicht heruntergeladen werden. Überspringe..."
      return 1
    fi
  fi
  
  chmod +x "$target"
  echo "Microsoft SBOM Tool installiert: $target"
}

run_ms_sbom() {
  echo "Starte Microsoft SBOM Scan in: $PROJECT_DIR"
  
  # Microsoft sbom-tool erwartet -BuildDropPath und -PackageName
  local build_path="$PROJECT_DIR"
  local package_name="$(basename "$PROJECT_DIR")"
  local manifest_dir="$DESKTOP/ms-sbom-manifest"
  
  mkdir -p "$manifest_dir"
  
  # Tool kann mit Exit-Code != 0 enden, trotzdem Manifest erstellen
  sbom-tool generate \
    -BuildDropPath "$build_path" \
    -BuildComponentPath "$build_path" \
    -PackageName "$package_name" \
    -PackageVersion "1.0.0" \
    -PackageSupplier "Organization: Local" \
    -ManifestDirPath "$manifest_dir" \
    -Verbosity Information 2>/dev/null || true
  
  # Manifest-Datei nach Desktop kopieren (auch wenn Warnungen auftraten)
  if [[ -f "$manifest_dir/_manifest/spdx_2.2/manifest.spdx.json" ]]; then
    cp "$manifest_dir/_manifest/spdx_2.2/manifest.spdx.json" "$MS_OUT"
    echo "✅ Microsoft SBOM Scan abgeschlossen: $MS_OUT"
    rm -rf "$manifest_dir"
  else
    echo "⚠️ Microsoft SBOM Manifest nicht gefunden (überspringe)."
    rm -rf "$manifest_dir"
    return 1
  fi
}

# Hauptablauf
if [[ ! -d "$PROJECT_DIR" ]]; then
  echo "Fehler: Projektpfad nicht gefunden: $PROJECT_DIR" >&2
  exit 1
fi

echo "=========================================="
echo "Security & SBOM Scan"
echo "=========================================="
echo "Projekt:   $PROJECT_DIR"
echo "Scan-Typ:  $SCAN_TYPE"
echo "Ausgabe:   ~/Desktop/"
echo "=========================================="
echo ""

# Installation der benötigten Tools
case "$SCAN_TYPE" in
  all|sbom|vuln|secret|config|license)
    install_trivy
    ;;
esac

case "$SCAN_TYPE" in
  all|sbom)
    install_cdxgen
    install_ms_sbom
    ;;
esac

# Scan-Ausführung basierend auf Typ
case "$SCAN_TYPE" in
  all)
    echo "Führe alle Scans aus..."
    run_trivy_sbom || true
    run_trivy_vulnerabilities || true
    run_trivy_secrets || true
    run_trivy_misconfig || true
    run_trivy_licenses || true
    run_trivy_full_report || true
    run_cdxgen || true
    run_ms_sbom || true
    
    echo ""
    echo "=========================================="
    echo "✅ Alle Scans abgeschlossen"
    echo "=========================================="
    echo "Trivy:"
    echo "  - SBOM:              $TRIVY_SBOM_OUT"
    echo "  - Vulnerabilities:   $TRIVY_VULN_OUT"
    echo "  - Secrets:           $TRIVY_SECRET_OUT"
    echo "  - Misconfigurations: $TRIVY_CONFIG_OUT"
    echo "  - Licenses:          $TRIVY_LICENSE_OUT"
    echo "  - HTML-Report:       $TRIVY_REPORT_HTML"
    echo ""
    echo "SBOM-Tools:"
    echo "  - cdxgen:            $CDX_OUT"
    echo "  - Microsoft:         $MS_OUT"
    echo "=========================================="
    ;;
    
  sbom)
    echo "Führe SBOM-Scans aus..."
    run_trivy_sbom || true
    run_cdxgen || true
    run_ms_sbom || true
    
    echo ""
    echo "=========================================="
    echo "✅ SBOM-Scans abgeschlossen"
    echo "=========================================="
    echo "  - Trivy:     $TRIVY_SBOM_OUT"
    echo "  - cdxgen:    $CDX_OUT"
    echo "  - Microsoft: $MS_OUT"
    echo "=========================================="
    ;;
    
  vuln)
    echo "Führe Vulnerability-Scan aus..."
    run_trivy_vulnerabilities || true
    run_trivy_full_report || true
    
    echo ""
    echo "=========================================="
    echo "✅ Vulnerability-Scan abgeschlossen"
    echo "=========================================="
    echo "  - JSON:        $TRIVY_VULN_OUT"
    echo "  - HTML-Report: $TRIVY_REPORT_HTML"
    echo "=========================================="
    ;;
    
  secret)
    echo "Führe Secret-Detection aus..."
    run_trivy_secrets || true
    
    echo ""
    echo "=========================================="
    echo "✅ Secret-Detection abgeschlossen"
    echo "=========================================="
    echo "  - Secrets: $TRIVY_SECRET_OUT"
    echo "=========================================="
    ;;
    
  config)
    echo "Führe Misconfiguration-Scan aus..."
    run_trivy_misconfig || true
    
    echo ""
    echo "=========================================="
    echo "✅ Misconfiguration-Scan abgeschlossen"
    echo "=========================================="
    echo "  - Misconfigurations: $TRIVY_CONFIG_OUT"
    echo "=========================================="
    ;;
    
  license)
    echo "Führe License-Scan aus..."
    run_trivy_licenses || true
    
    echo ""
    echo "=========================================="
    echo "✅ License-Scan abgeschlossen"
    echo "=========================================="
    echo "  - Licenses: $TRIVY_LICENSE_OUT"
    echo "=========================================="
    ;;
    
  *)
    echo "Fehler: Unbekannter Scan-Typ: $SCAN_TYPE" >&2
    usage
    exit 1
    ;;
esac
