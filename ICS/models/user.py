from db.db import db

class User(db.Model):
  __tablename__ = 'users'

  id = db.Column(db.Integer, primary_key=True, autoincrement = True)
  username = db.Column(db.String(100), nullable=False)
  password = db.Column(db.String(100), nullable=False)