import os
import smtplib
import threading
import time
from email.mime.text import MIMEText

from flask import Flask, jsonify, request


SMTP_HOST = os.environ.get("CLOUDLOCK_SMTP_HOST", "smtp.gmail.com")
SMTP_PORT = int(os.environ.get("CLOUDLOCK_SMTP_PORT", "587"))
MAIL_USERNAME = os.environ.get("CLOUDLOCK_MAIL_USERNAME", "")
MAIL_PASSWORD = os.environ.get("CLOUDLOCK_MAIL_PASSWORD", "")
MAIL_TO = os.environ.get("CLOUDLOCK_MAIL_TO", "")
ALERT_SHARED_KEY = os.environ.get("CLOUDLOCK_ALERT_KEY", "")
ALERT_EMAIL_TEXT = "ALERT !!!  The object has been stolen !"


class EmailNotifier:
    def __init__(self):
        self.last_message = "Email not configured"
        self.lock = threading.Lock()

    def is_configured(self):
        return MAIL_USERNAME != "" and MAIL_PASSWORD != "" and MAIL_TO != ""

    def send_stolen_alert(self):
        if not self.is_configured():
            with self.lock:
                self.last_message = "Email not configured"
            return

        message = MIMEText(ALERT_EMAIL_TEXT)
        message["Subject"] = "Cloudlock security alert"
        message["From"] = MAIL_USERNAME
        message["To"] = MAIL_TO

        try:
            with smtplib.SMTP(SMTP_HOST, SMTP_PORT, timeout=12) as smtp:
                smtp.starttls()
                smtp.login(MAIL_USERNAME, MAIL_PASSWORD)
                smtp.send_message(message)
            with self.lock:
                self.last_message = f"Alert sent to {MAIL_TO} at {time.strftime('%H:%M:%S')}"
        except Exception as exc:
            with self.lock:
                self.last_message = f"Email error: {exc}"

    def send_stolen_alert_async(self):
        thread = threading.Thread(target=self.send_stolen_alert, daemon=True)
        thread.start()

    def get_last_message(self):
        with self.lock:
            return self.last_message


app = Flask(__name__)
email_notifier = EmailNotifier()


@app.get("/")
def index():
    return jsonify({
        "service": "Cloudlock WiFi email gateway",
        "status": "running",
        "email_configured": email_notifier.is_configured(),
        "email_to": MAIL_TO,
        "last_email_status": email_notifier.get_last_message(),
    })


@app.get("/api/health")
def api_health():
    return jsonify({
        "ok": True,
        "email_configured": email_notifier.is_configured(),
        "email_status": email_notifier.get_last_message(),
    })


@app.post("/api/wifi-alert")
def api_wifi_alert():
    if not ALERT_SHARED_KEY:
        return jsonify({"ok": False, "error": "Alert key is not configured"}), 503

    received_key = request.headers.get("X-Cloudlock-Key", "")
    if ALERT_SHARED_KEY and received_key != ALERT_SHARED_KEY:
        return jsonify({"ok": False, "error": "Invalid alert key"}), 401

    body = request.get_json(silent=True) or {}
    event = str(body.get("event", "")).upper()

    if event != "OBJECT_STOLEN":
        return jsonify({"ok": False, "error": "Invalid event"}), 400

    email_notifier.send_stolen_alert_async()
    return jsonify({"ok": True, "message": "Alert email queued"})


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True, use_reloader=False)
