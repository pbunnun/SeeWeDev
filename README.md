See-We-Dev  --  Flow-Based Visual Programming for OpenCV. CVDev has been designed and developped as a software tool so that developers can reuse their codes and share their work with others. 

CVDev เป็นโปรแกรมที่ช่วยในการพัฒนาแอพพลิเคชั่นในลักษณะที่เรียกว่า Flow-Based Visual Programming การพัฒนาแอพพลิเคชั่น สามารถทำได้โดยการลากกล่องหรือ Node ต่างๆ และนำมาสร้างเป็นแผนภาพ โดยข้อมูลจะไหลจาก Node หนึ่งไปยังอีก Node หนึ่งตามเส้นทางที่ผู้ใช้งานลากเส้นทางเชื่อมโยง Node แต่ละ Node ไว้ โดยในแต่ละ Node จะทำการประมวลผลข้อมูลที่ได้รับและส่งผลการประมวลต่อไปให้กับ Node ในลำดับถัดไป จนในที่สุดได้ผลลัพธ์ของงานที่ต้องการ โดยที่ไม่จำเป็นต้องคอมไพล์โปรแกรมใหม่

หน่วยประมวลผลย่อยต่างๆ จะถูกพัฒนาให้เป็น Node ที่สามารถเรียกใช้งานด้วย CVDev ได้ ผู้ใช้งานสามารถนำหน่วยประมวลผลที่รับและส่งข้อมูลประเภทเดียวกันมาเชื่อมต่อให้ทำงานร่วมกันได้โดยการลากเส้นทางเชื่อมโยงกันระหว่างหน่วยประมวลผลใน CVDev ส่งผลให้กระบวนการพัฒนาโปรแกรมต้นแบบโดยรวมจะทำได้รวดเร็วขึ้น โดยเฉพาะงานที่สามารถเรียกใช้หน่วยประมวลผลเดิมที่เคยพัฒนาไว้ได้

ในการใช้งานนั้น Node ที่ถูกพัฒนาขึ้น จะถูกจัดเก็บรวบรวมอยู่ในไฟล์ Plugin ที่ CVDev สามารถโหลด Node ที่อยู่ในไฟล์นั้นๆ ขึ้นมาใช้งานได้ ทำให้การแจกจ่าย Node เพื่อนำไปใช้งาน สามารถแจกจ่ายในรูปของไฟล์ Plugin ได้ โดยที่ CVDev สามารถโหลดไฟล์ Plugin ได้ทีละหลายๆ ไฟล์พร้อมกัน และ สามารถบันทึกและโหลด แผนภาพการเชื่อมต่อ Node เพื่อเรียกใช้งานในภายหลังได้

CVDev และ Node พื้นฐานต่างๆ ถูกพัฒนาขึ้นโดยใช้ภาษา C++ เพื่อให้ CVDev สามารถทำการประมวลผลได้อย่างรวดเร็ว ในการพัฒนา Node นั้นสามารถเริ่มต้นพัฒนาได้จาก Template ที่เตรียมไว้ให้ ไม่จำเป็นต้องพัฒนาทุกส่วนเองทั้งหมด ทั้งนี้เพื่อให้นักพัฒนาได้มีเวลาไปมุ้งเน้นในการพัฒนาวิธีประมวลผลที่อยู่ใน Node นั้นๆ!

[![Blood Type Test](https://img.youtube.com/vi/cvRiDyQiHgA/0.jpg)](https://www.youtube.com/watch?v=cvRiDyQiHgA)

[![Blood Type Test](https://img.youtube.com/vi/4ygVRSnO750/0.jpg)](https://www.youtube.com/watch?v=4ygVRSnO750)

[![Blood Type Test](https://img.youtube.com/vi/PIxWVjwQGSs/0.jpg)](https://www.youtube.com/watch?v=PIxWVjwQGSs)

[![Blood Type Test](https://img.youtube.com/vi/fa7johYZEeQ/0.jpg)](https://www.youtube.com/watch?v=fa7johYZEeQ)

[![Blood Type Test](https://img.youtube.com/vi/cNzBbehysp4/0.jpg)](https://www.youtube.com/watch?v=cNzBbehysp4)

CVDev is made possible by open source softwares mainly NodeEditor, Qt and OpenCV.

NodeEditor : https://github.com/paceholder/nodeeditor

Qt : https://www.qt.io 

OpenCV : https://opencv.org
