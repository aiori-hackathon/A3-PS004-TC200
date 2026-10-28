# DNSGuard — AIORI-3 PS-004 Dashboard

Responsive React + Vite frontend for **Gateway-Assisted DNSSEC for ESP8266**.

## Demo-mode disclosure
This frontend is intentionally **not connected to a live ESP8266, Unbound, or NSD service**. Dashboard metrics, query history, event logs, and the test runner use illustrative local data. The UI labels them as demo/simulated. No live integration or successful live test is claimed.

## Stack
- React 18 + Vite
- Recharts sample query chart
- Lucide React icons
- Responsive CSS

## Run locally
Requirements: Node.js 18+ and npm.

```bash
npm install
npm run dev
```
Open the local URL Vite prints (typically `http://localhost:5173`).

Production build:

```bash
npm run build
npm run preview
```

## Pages
- **Overview** — demo metrics, sample query chart, expected DNSSEC distribution and request path.
- **DNSSEC Test Console** — interactive simulated scenarios for `secure.lab.test`, `insecure.lab.test`, and `www.bogus.lab.test`.
- **Query Explorer** — search/filter sample records and export them as CSV.
- **System Architecture** — conceptual ESP8266 → DoT → Unbound → NSD test-zone flow.
- **Event Logs** — illustrative events, explicitly not device logs.

## Expected test scenarios
| Domain | Expected response | Expected classification |
|---|---|---|
| `secure.lab.test` | `NOERROR` + AD flag | SECURE |
| `insecure.lab.test` | `NOERROR`, AD absent | INSECURE |
| `www.bogus.lab.test` | `SERVFAIL` | BOGUS |

These are expected outcomes represented by the UI, not proof that a live test passed.

## Future backend integration
Implement a backend API for actual resolver health and query outcomes. Replace fixture arrays in `src/App.jsx` with API data and add loading, stale-data, and offline states. Never put PSKs, private keys, or gateway credentials in frontend source code.

AIORI-3 · Team TC200 · PS-004 — Gateway-Assisted DNSSEC for ESP8266
