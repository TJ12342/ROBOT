from flask import Flask, jsonify, request
from flask_cors import CORS
from modules.user import user_bp
from modules.index import index_bp
from modules.face import face_bp
from modules.robot import robot_bp
from db.db import db

app = Flask(__name__)

CORS(app)

HOSTNAME = "101.132.80.183"
PORT = 5433
USERNAME = "qcit5688"
PASSWORD = "Qcit5688"
DATABASE = "dbfe23d4140bf64b55ae7c6289a3cc56acusers"
app.secret_key = "liujiaxin@lagowangdemo"

# 数据库配置
app.config['SQLALCHEMY_DATABASE_URI'] = f"postgresql://{USERNAME}:{PASSWORD}@{HOSTNAME}:{PORT}/{DATABASE}"

app.register_blueprint(user_bp, url_prefix='/user')
app.register_blueprint(index_bp, url_prefix='/')
app.register_blueprint(face_bp, url_prefix='/face')
app.register_blueprint(robot_bp, url_prefix='/robot')

db.init_app(app)

with app.app_context():
  # 手动创建新表
  db.create_all()


if __name__ == "__main__":
  app.run(debug=True)