Main: Ham chay khoi dong.
     Cap phat mot doi tuong gb.
     Khoi tao bo dem do hoa
     Khoi tao SDL
     Khoi tao cac thanh pha:
          Rom file
          Trang thai dong bo
          CPU, GPU
          Input, timer, SPU
     Gan cac gia tri khoi dau
     Bat dau vong lap game

GB: Struct ket tap cac thanh phan cua he thong
     Trang thai
     Toc do
     Thoi gian chay
     irq ?
     Frontend
     sync
     CPU, cart, gpu, input, dma, hdma, timer, spu
     Ram.

Cart:
     Cac vung nho rom, ram mo rong.
     Loai cart.
     Ho tro luu doc thong tin xuong cart.
// Doc them ve cac loai cart.
  Cart co ram + battery luu tru

CPU:
     Tich hop thanh ghi
     Lenh reset cpu.
          Gan gia tri mac dinh cho cac thanh ghi
          Khong dung boot rom
     Chay ma gia lap.
     Cac phuong thuc chung doc ghi byte, doc ghi thanh ghi, dump,
     Impliment Opcode leve1

DMA: ???

Frontend:
     Cac ham dieu khien output: DMG, GBC,
  Tap cac con tro ham doc lap nen tang cho phep thuc goi cac ham do hoa
  Draw line, flip, input, destroy

GPU:
     Bang mau.
     Cac thanh ghi
     Cac ham dieu khien

HDMA ???

Input:
    Thanh ghi cac trang thai dau vao.
    Cap nha trang thai cac thanh ghi.

IRQ ???

Memory:
    Doc ghi cac vung nho khac nhau phu thuoc vao dia chi

RTC:
    Module thoi gian

Timer
