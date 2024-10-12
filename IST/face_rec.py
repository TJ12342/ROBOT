import dlib
import cv2
import numpy as np
import os
import mysql.connector
from concurrent.futures import ThreadPoolExecutor


def face_detect(img):
    if img is None:
        raise ValueError("Image not loaded properly")

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    detector = dlib.get_frontal_face_detector()

    faces = detector(gray)
    if not faces:
        print("no face detected")
        return False
    else:
        print("face detected")
        return True


def face_recognition(img):
    predictor_path = "properties\\shape_predictor_68_face_landmarks.dat"
    face_rec_model_path = "properties\\dlib_face_recognition_resnet_model_v1.dat"
    if not os.path.exists(predictor_path):
        raise FileNotFoundError(f"Predictor file not found at {predictor_path}")
    if not os.path.exists(face_rec_model_path):
        raise FileNotFoundError(f"Face recognition model file not found at {face_rec_model_path}")
    if img is None:
        raise ValueError("Image not loaded properly")

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    detector = dlib.get_frontal_face_detector()
    predictor = dlib.shape_predictor(predictor_path)
    face_rec_model = dlib.face_recognition_model_v1(face_rec_model_path)

    conn = mysql.connector.connect(
        host='localhost',
        port='3306',
        user='root',  # 这里写你的数据库用户名
        password='',  # 这里写你数据库的密码
        database='faces'
    )

    faces = detector(gray)
    if not faces:
        cv2.namedWindow("Image", cv2.WINDOW_NORMAL)
        cv2.resizeWindow("Image", 800, 600)
        cv2.imshow("Image", img)
        print("No faces detected")
        conn.close()
    else:
        for face in faces:
            left = face.left()
            right = face.right()
            top = face.top()
            bottom = face.bottom()
            cv2.rectangle(img, (left, top), (right, bottom), (0, 255, 0), 3)
            shape = predictor(gray, face)
            face_descriptor = face_rec_model.compute_face_descriptor(img, shape)
            face_descriptor_np = np.array(face_descriptor)
            cursor = conn.cursor()
            cursor.execute('''
                SELECT id, name, features FROM face_features
            ''')
            flag = False
            for (id, name, features_bytes) in cursor:
                stored_face_descriptor_np = np.frombuffer(features_bytes, dtype=np.float64)
                similarity = np.linalg.norm(face_descriptor_np - stored_face_descriptor_np)
                threshold = 0.5
                if similarity < threshold:
                    print(f"Detected face matches with {name} (ID: {id}), similarity: {similarity}")
                    cv2.putText(img, name, (left, bottom + 45), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 255, 0), 3)
                    flag = True
            if not flag:
                # 传输陌生人人脸坐标
                cv2.putText(img, "Unknown", (left, bottom + 45), cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 255, 0), 3)
                print("Unknown people")

        conn.commit()
        conn.close()

        cv2.namedWindow("Image with faces", cv2.WINDOW_NORMAL)
        cv2.resizeWindow("Image with faces", 800, 600)
        cv2.imshow("Image with faces", img)
        cv2.waitKey(0)
        cv2.destroyAllWindows()


def main(img):
    with ThreadPoolExecutor() as executor:
        future = executor.submit(face_detect, img)
        executor.submit(face_recognition, img)
        detection_result = future.result()
    return detection_result


if __name__ == "__main__":
    img_path = "imgs\\12236_S.jpg"
    img = cv2.imread(img_path)
    result = main(img)
    print(result)
