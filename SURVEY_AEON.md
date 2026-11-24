# SURVEY_AEON.md

## 1. Phạm vi và nguyên tắc sử dụng dữ liệu

Dự án mô phỏng **AEON MALL Đà Nẵng Thanh Khê** tại số 46 Điện Biên Phủ, phường Thanh Khê, Đà Nẵng. Đây là AEON MALL nằm trong khối đế TTC Plaza Đà Nẵng, không phải dự án AEON MALL Đà Nẵng Hòa Xuân.

Tài liệu này ghi lại dữ liệu đầu vào do người thực hiện cung cấp. Các liên kết tham khảo được giữ nguyên theo dữ liệu đầu vào; nội dung của từng nguồn chưa được xác minh độc lập trong lần cập nhật mã nguồn này. Phần mô phỏng giao thông dựng **B1 và B2 như hai thế giới demo độc lập**, mỗi tầng có một bố cục Zone A mẫu 64 ô ô tô. Đây là lựa chọn trình diễn, không phải khẳng định mỗi tầng thực tế có 64 ô theo cách sắp xếp này. Ngoại thất AEON, khu xe máy và lõi thang máy đều là hình học giả định, không phải bản dựng theo bản vẽ thi công.

Quy ước tọa độ trong code: `1 unit = 1 m`; X là chiều dài; Z là chiều rộng; Y là chiều cao; cao độ âm nằm dưới mặt đất.

Mỗi hằng số hình học được quản lý trong `aeon_layout.h` và có nhãn:

- `[SOURCE]`: số liệu công khai được chép từ dữ liệu đầu vào.
- `[DERIVED]`: giá trị tính từ công thức dựa trên dữ liệu nguồn hoặc giả định đã nêu.
- `[ASSUMED]`: giả định thiết kế / tham chiếu mô phỏng, không phải số đo đã khảo sát tại AEON.

## 2. Dữ liệu công khai [SOURCE]

| Hạng mục | Giá trị đầu vào | Phạm vi / ghi chú |
|---|---:|---|
| Địa chỉ | 46 Điện Biên Phủ, Thanh Khê, Đà Nẵng | TTC Plaza Đà Nẵng |
| Diện tích khu đất | Gần 15.000 m² | Toàn tổ hợp TTC Plaza |
| Tổng diện tích sàn | Hơn 122.000 m² | Toàn tổ hợp |
| Quy mô | 18 tầng nổi + 2 tầng hầm | Toàn tổ hợp |
| Khối đế thương mại | 4 tầng | Dùng chung khối đế |
| Diện tích sàn thương mại AEON | Gần 30.000–31.000 m² | Các nguồn nêu khoảng giá trị khác nhau |
| GLA | Khoảng 21.000 m² | Có nguồn khác nêu khoảng 30.000 m²; đang có chênh lệch |
| Diện tích bãi đỗ xe | Gần 23.000 m² | Toàn tổ hợp, không phải riêng ô tô hoặc riêng B1 |
| Sức chứa ô tô | Khoảng 430 chỗ | Tổng hai tầng hầm của tổ hợp |
| Sức chứa xe máy | Khoảng 1.800 chỗ | Tổng hai tầng hầm của tổ hợp |
| Phí ô tô | 25.000 đồng / 2 giờ đầu; 10.000 đồng / mỗi giờ tiếp theo | Theo biểu phí được cung cấp |
| Phí xe máy | 4.000 đồng / 4 giờ đầu; 2.000 đồng / mỗi 2 giờ tiếp theo | Theo biểu phí được cung cấp |
| Hỗ trợ thanh toán lối ra | Nhân viên dùng thiết bị thanh toán di động hỗ trợ khách xếp hàng | Dữ liệu vận hành được cung cấp |
| Ngày khai trương | 03/07/2026 | Theo dữ liệu đầu vào |
| Ngày công bố biểu phí | 29/06/2026 | Theo dữ liệu đầu vào |

Các giá trị công khai này được lưu trong `AeonLayout` của `aeon_layout.h`. Chúng dùng để mô tả quy mô công trình và thông tin tham khảo, không được dùng để suy diễn sơ đồ mặt bằng chi tiết.

## 3. Giá trị suy ra [DERIVED]

| Đại lượng | Công thức | Kết quả | Cách sử dụng |
|---|---|---:|---|
| Diện tích bãi đỗ bình quân mỗi tầng hầm | 23.000 / 2 | ≈ 11.500 m²/tầng | Chỉ tham khảo quy mô; không phải diện tích thực đo của B1 |
| Xe máy trên mỗi ô tô theo sức chứa | 1.800 / 430 | ≈ 4,2 | Tỷ lệ sức chứa toàn tổ hợp |
| Diện tích bình quân tham chiếu mỗi chỗ ô tô (gồm lối đi) | Ước lượng trong dữ liệu đầu vào | ≈ 25–30 m²/xe | Giá trị tham khảo; phụ thuộc giả định ô tô chiếm phần lớn diện tích bãi, không phải phép đo riêng của AEON |
| Phần sức chứa ô tô đại diện của Zone A | 64 / 430 × 100 | ≈ 14,9% | Hiển thị thông tin `ZONE A SAMPLE 64 / APPROX 430 TOTAL` trên HUD |
| Chiều dài dốc theo độ dốc tham chiếu | 3,5 / 0,12 | ≈ 29,17 m | Dốc mô phỏng từ cao độ 0 đến B1 -3,5 m; phụ thuộc giả định độ dốc |
| Kích thước lõi đỗ xe | 40 × 32 | 1.280 m² | Hình học giả định cho Zone A |
| Kích thước khu Zone A gồm làn chu vi | 52 × 44 | 2.288 m² | 40 + 2×6 theo X; 32 + 2×6 theo Z |
| Số ô trong một dãy | 40 / 2,5 | 16 | Số ô mỗi hàng |
| Tổng ô Zone A | 2 mô-đun × 2 hàng × 16 | 64 | Ô tô con |
| Bước cột | 3 × 2,5 | 7,5 m | Theo lưới tham chiếu |

Lưu ý quan trọng: **23.000 m²** là diện tích bãi đỗ xe của toàn tổ hợp, có thể bao gồm đường dốc, lõi thang, phòng kỹ thuật và khu xe máy. Không được coi 11.500 m²/tầng là diện tích sàn ô tô thuần hoặc lấy con số này làm mặt bằng thật của B1.

## 4. Giả định thiết kế cho Zone A [ASSUMED]

### 4.1 Xe mẫu

| Hằng số trong code | Giá trị | Ghi chú |
|---|---:|---|
| `CAR_LENGTH_M` | 4,41 m | Tham chiếu Toyota Vios; cần đối chiếu thế hệ xe thực tế |
| `CAR_WIDTH_M` | 1,74 m | Tham chiếu Toyota Vios |
| `CAR_HEIGHT_M` | 1,48 m | Tham chiếu Toyota Vios |
| `CAR_WHEELBASE_M` | 2,55 m | Tham chiếu Toyota Vios |

### 4.2 Ô đỗ, làn, cột và cao độ

| Hằng số | Giá trị | Ghi chú |
|---|---:|---|
| `STALL_W_M` | 2,5 m | Rộng ô |
| `STALL_L_M` | 5,0 m | Dài ô |
| `STALL_ANGLE_DEG` | 90° | Đỗ vuông góc |
| `AISLE_W_M` | 6,0 m | Lối đi hai chiều tham chiếu |
| `PERIMETER_LANE_W_M` | 6,0 m | Làn chu vi |
| `COLUMN_SIZE_M` | 0,6 × 0,6 m | Tiết diện cột |
| `STALLS_PER_COLUMN_SPAN` | 3 ô | Bước cột theo 3 ô |
| `COLUMN_SPACING_X_M` | 7,5 m | Suy ra từ 3 × 2,5 m |
| `FLOOR_TO_CEILING_M` | 3,0 m | Chiều cao tầng tham chiếu |
| `FLOOR_HEIGHT_B1_M` | -3,5 m | Cao độ B1 giả định |
| `FLOOR_HEIGHT_B2_M` | -7,0 m | Cao độ B2 giả định; B2 được dựng thành world demo riêng |
| `RAMP_SLOPE` | 12% | Độ dốc tham chiếu |
| `RAMP_LANE_W_M` | 3,5 m/làn | Làn vào và ra tách riêng |
| `BARRIER_ARM_LEN_M` | 3,0 m | Chiều dài cần chắn tham chiếu |

### 4.3 Bố cục 2 mô-đun Zone A

- Hai mô-đun song song theo Z. Mỗi mô-đun rộng `5 + 6 + 5 = 16 m`, gồm hai dãy ô đỗ và một lối đi ở giữa.
- Mỗi dãy dài 40 m, chia thành 16 ô × 2,5 m.
- Tổng cộng `2 mô-đun × 2 dãy × 16 ô = 64 ô ô tô`.
- Lõi đỗ xe có kích thước 40 × 32 m.
- Làn chu vi giả định rộng 6 m mỗi phía, tạo khu Zone A 52 × 44 m.
- Hai làn dốc song song, mỗi làn rộng 3,5 m, đặt tại cạnh phía Tây của khu đại diện. Vị trí cụ thể là quyết định thiết kế, không phải vị trí cổng thực tế của AEON.
- Hình học cột sinh từ bước 7,5 m tại các cạnh lối đi. Với chiều dài 40 m, chỉ có 5 khoảng đủ 7,5 m (37,5 m) và phần cuối còn 2,5 m. Code giữ bước 7,5 m như dữ liệu đầu vào; không tự điều chỉnh thành bước khác.
- Các làn trong mỗi lối đi được tách thành một hướng vào và một hướng ra để giảm xung đột trong bản demo. Đây là quy tắc điều phối mô phỏng, không phải xác nhận tổ chức giao thông thực tế.

### 4.4 Các thông số chỉ phục vụ hoạt ảnh

Tốc độ xe, thời gian chờ cổng, thời gian đỗ ngẫu nhiên 90–120 giây, nhịp sinh xe 1,5 giây, khoảng cách an toàn, số xe tối đa đồng thời, góc camera, kích thước HUD và độ dày đường kẻ đều được gắn `[ASSUMED]` trong `aeon_layout.h`. Đây là thông số phần mềm để chạy demo, không được trình bày là tiêu chuẩn vận hành hoặc số đo của AEON.

### 4.5 Massing ngoại thất

- Mô hình tham khảo khối đế thương mại 4 tầng và hai tháp phía trên theo thông tin quy mô được cung cấp; cách chia tầng cao, số tháp, vị trí và kích thước từng khối là giả định phục vụ nhận diện hình dáng.
- Mặt tiền kính, các dải ngang uốn lượn, cổng vòm, bảng AEON MALL và hàng cây được dựng bằng hình học khối đơn giản trong `AeonMallExterior`.
- Kích thước tổng thể, cao độ từng tầng, vật liệu/màu sắc và chi tiết mặt đứng chưa được đo đạc hoặc đối chiếu với bản vẽ kiến trúc chính thức.
- Phím `V` ẩn/hiện ngoại thất để chuyển giữa góc nhìn công trình và B1; phím `1` chọn B1, phím `2` chọn B2. Các thế giới tầng hầm giữ trạng thái riêng.

### 4.6 Khu xe máy và lối tiếp cận [ASSUMED]

- Mô hình tạo một annex xe máy minh họa bên ngoài ranh 52 × 44 m của Zone A ô tô, ở phía Bắc theo trục Z.
- Annex có 2 hàng × 12 ô = 24 ô xe máy demo trên mỗi tầng được mô phỏng; mỗi ô 0,85 × 2,0 m, lối cách hàng và lối nối tới khu chính đều là giả định.
- 8 mô hình xe máy tĩnh được đặt ban đầu. Phím `B` thêm một xe vào ô trống; phím `X` xóa xe cuối cùng. Đây là công cụ trình diễn trạng thái đỗ, không phải mô phỏng xe máy chạy vào/ra.
- Con số 1.800 xe máy là sức chứa ước lượng của toàn bộ tổ hợp và hai tầng hầm; không dùng để suy ra số ô của annex demo.

### 4.7 Lõi thang máy [ASSUMED]

- Mỗi tầng demo hiển thị một lõi gồm 2 giếng thang, cửa, sảnh và lối đi nối với ranh Zone A.
- Kích thước và tọa độ lõi, giếng thang, cửa và lối đi đều nằm trong `aeon_layout.h`, mang nhãn `[ASSUMED]` hoặc `[DERIVED]` từ giả định.
- Mô hình hiện chỉ là hình học tĩnh; chưa có cabin thang máy, hoạt ảnh cửa, di chuyển đứng giữa G/B1/B2 hoặc mô phỏng hành khách.

### 4.8 Tầng B2 [ASSUMED / DERIVED]

- B2 dùng cao độ `-7,0 m`; B1 dùng `-3,5 m`. Hai giá trị là giả định, không phải số đo công trình.
- Tạo một `ParkingWorld` riêng cho B2, có 64 ô xe hơi demo độc lập, annex 24 ô xe máy demo và lõi thang tĩnh giống bố cục trình diễn B1.
- Dốc B2 bắt đầu ở cao độ B1 và xuống B2 với độ giảm 3,5 m; chiều dài được suy ra từ độ dốc tham chiếu 12% (xấp xỉ 29,17 m).
- Phím `1` chọn B1; phím `2` chọn B2. Hai thế giới được cập nhật song song, nhưng trạng thái chỗ đỗ độc lập.

## 5. Những phần đã được tạo trong code

1. `aeon_layout.h`: tập trung dữ liệu công khai, giá trị suy ra, giả định, giới hạn mô hình và hàm sinh tọa độ.
2. `parking_main.cpp`: sinh 64 ô đỗ, cột, vạch, làn, mũi tên, đường dốc, barrier, vị trí camera và waypoint từ `AeonLayout`.
3. Ô đỗ có ba trạng thái `Free`, `Reserved`, `Occupied`.
4. Xe tự động chạy từ dốc vào, qua barrier, vào làn, tiếp cận ô, lùi vào ô, chờ, tiến ra, đến barrier ra, chờ mô phỏng thanh toán rồi chạy lên dốc.
5. Hai luồng trong aisle được tách thành đường vào và đường ra; va chạm được kiểm tra bằng hình chữ nhật bao đơn giản theo hướng xe.
6. HUD hiển thị `ZONE A SAMPLE 64 / APPROX 430 TOTAL`, số xe, số ô trống/đặt trước/đã chiếm, trạng thái FULL, hàng đợi thanh toán và cảnh báo bố cục giả định.
7. Mô hình xe được giữ theo kích thước tham chiếu Vios và bỏ bảng/đèn nóc kiểu xe tập lái để phù hợp hơn với xe con thông thường.
8. `scene_view.h`: mô hình ngoại thất AEON dạng massing, gồm khối đế, hai tháp, mặt tiền uốn lượn, cổng vòm, bảng hiệu và phố cây xanh; có thể bật/tắt riêng với B1.

## 6. Danh sách chưa đo được / chưa xác minh tại hiện trường

Các dữ liệu dưới đây **không tìm thấy trong bộ dữ liệu công khai đã cung cấp** và vẫn phải gắn `[ASSUMED]` cho đến khi khảo sát hoặc nhận được bản vẽ có thẩm quyền:

- Bản vẽ mặt bằng chính thức của riêng B1 và riêng B2.
- Ranh giới, kích thước thực tế và hình dạng từng khu vực của tầng hầm.
- Vị trí, số lượng và lưới cột thực tế; tiết diện cột thực tế.
- Kích thước ô đỗ thực tế, góc đỗ và số lượng ô ô tô/xe máy theo từng tầng.
- Bề rộng, chiều lưu thông và phân làn thực tế của từng lối đi.
- Vị trí, số lượng, hướng di chuyển, chiều dài và độ dốc thực tế của đường dốc.
- Vị trí và phân luồng thực tế của cổng vào/cổng ra; vị trí barrier và kích thước thiết bị.
- Vị trí, diện tích, số ô và cách phân luồng thực tế của khu xe máy trên từng tầng; kích thước các ô xe máy trong mô hình hiện tại chỉ là giả định.
- Vị trí, kích thước, số lượng và hướng mở cửa của lõi thang máy thực tế; mô hình hai giếng thang trong bản demo không phải thông tin khảo sát.
- Vị trí khu dành cho xe đặc biệt, khu đón/trả khách.
- Vị trí thang cuốn, thang bộ, phòng kỹ thuật và lối thoát hiểm.
- Biển báo, vạch sơn, ký hiệu tầng/khu, đèn, màu sắc và quy định giao thông nội bộ.
- Chiều cao trần thông thủy thực tế và các vật cản kỹ thuật dưới trần.
- Thế hệ Toyota Vios cụ thể dùng làm xe mẫu và việc đối chiếu thông số xe.
- Kích thước mặt bằng, chiều cao, hình khối tháp, mặt đứng, vật liệu, cửa vào và cảnh quan ngoại thất thực tế.

## 7. Giới hạn của mô phỏng hiện tại

- Mô phỏng giao thông chỉ dựng B1 – Zone A đại diện 64 ô tô, không dựng toàn bộ 430 ô ô tô và 1.800 ô xe máy của toàn tổ hợp. Ngoại thất chỉ là massing nhận diện, không theo tỷ lệ khảo sát.
- Sơ đồ 52 × 44 m, vị trí ramp, gate, columns và các làn là giả định thiết kế; không được gọi là mặt bằng đo được của AEON.
- B2 có cảnh mô phỏng riêng với 64 ô demo, ramp B1–B2 và annex phụ. Đây là bố cục lặp lại để kiểm thử/tương tác; không phải bản vẽ B2 thật hoặc thông tin phân bố 430 ô xe của tổ hợp.
- Hàng đợi ra cổng chỉ mô phỏng thời gian chờ hỗ trợ thanh toán; chưa tích hợp thanh toán thật, không tính phí tự động theo thời gian thực và không kết nối thiết bị.
- Kiểm tra va chạm xe dùng footprint hình chữ nhật gần đúng theo hướng xe và dung sai cao độ trên dốc; không phải physics engine hoặc thuật toán tìm đường tối ưu, cũng chưa đảm bảo tránh deadlock ở mọi tình huống.
- Xe máy là mô hình tĩnh có thể thêm/xóa; chưa có route xe máy hoặc logic xe máy ra/vào.
- Lõi thang máy là hình học tĩnh; chưa có cabin, hoạt ảnh cửa hoặc chuyển động đứng.
- Bộ headless test dùng GLM stub cho `ParkingWorld`, không build renderer. Bản desktop vẫn cần build và chạy thử trên Windows có OpenGL, GLFW, GLEW và GLM.

## 8. Nguồn tham khảo được cung cấp

Các URL dưới đây được giữ nguyên theo dữ liệu người thực hiện cung cấp; hãy kiểm tra nội dung và ngày đăng khi trích dẫn trong báo cáo chính thức.

1. [Thị trường Tài chính Tiền tệ – TTC Plaza Đà Nẵng chính thức vận hành… (03/07/2026)](https://thitruongtaichinhtiente.vn/ttc-plaza-da-nang-chinh-thuc-van-hanh-voi-dau-moc-khai-truong-aeon-mall-84028.html)
2. [TTC Group – AEON MALL shopping center officially opens at TTC Plaza Da Nang](https://www.ttcgroup.vn/en-US/business-sectors/real-estate/other-news-1/aeon-mall-shopping-center-officially-opens-at-ttc-plaza-da-nang)
3. [Đà Nẵng Fantasticity – AEON MALL Da Nang Thanh Khe](https://danangfantasticity.com/en/discovery/aeon-mall-da-nang-thanh-khe-the-first-aeon-mall-in-da-nang-and-the-eighth-aeon-mall-in-vietnam)
4. [AEON MALL Đà Nẵng Thanh Khê – Hướng dẫn gửi xe và biểu phí (29/06/2026)](https://danangthanhkhe.aeonmall-vietnam.com/en/news/dao-mall-thanh-thoi-huong-dan-gui-xe-va-bieu-phi-tai-aeon-mall-da-nang-thanh-khe.html)
5. [VOV – AEON MALL Đà Nẵng Thanh Khê: Điểm đến mới cho nhịp sống đô thị miền Trung](https://vov.vn/doanh-nghiep/doanh-nghiep-24h/eon-mall-da-nang-thanh-khe-diem-den-moi-cho-nhip-song-do-thi-mien-trung-post1312777.vov)
6. [Đại biểu Nhân dân – Người dân Đà Nẵng hưởng lợi gì trong tuần lễ khai trương AEON MALL](https://daibieunhandan.vn/nguoi-dan-da-nang-huong-loi-gi-trong-tuan-le-khai-truong-aeon-mall-10422610.html)


## 9. Source of numeric assumptions

The complete set of source facts, derived values and key `[ASSUMED]` dimensions is maintained in `aeon_layout.h`; the README appendix summarizes the most important values. Remaining decorative offsets and materials in the renderer are visual approximations, not AEON measurements.
