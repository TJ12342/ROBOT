from db.db import db
from sqlalchemy import func

class FaceFeature(db.Model):
    __tablename__ = 'face_features'

    id = db.Column(db.Integer, primary_key=True, autoincrement=True)
    name = db.Column(db.String(255), nullable=False)
    gender = db.Column(db.String(6), nullable=False)  # 使用字符串并添加 CHECK 约束
    department = db.Column(db.String(255), nullable=True)
    major = db.Column(db.String(255), nullable=True)
    class_ = db.Column('class', db.String(255), nullable=True)  # 使用 class_ 避免与关键字冲突
    features = db.Column(db.LargeBinary, nullable=False)  # 使用 LargeBinary 存储 BYTEA 数据
    created_at = db.Column(db.DateTime, default=func.now(), nullable=False)

    __table_args__ = (
        db.CheckConstraint(gender.in_(['Male', 'Female']), name='check_gender'),
    )

