// RUN: %cheri_purecap_cc1 -emit-llvm -cheri-linker -o - %s | FileCheck %s
// RUN: %cheri_purecap_cc1 -emit-obj -cheri-linker -o - %s | llvm-readobj -r - | FileCheck -check-prefix=RELOCS %s

class A {
public:
  int nonvirt() { return 1; }
  int nonvirt2() { return 2; }
  virtual int virt() { return 3; }
  virtual int virt2() { return 4; }
};
int global_fn() { return 5; }

typedef int (A::* MemberPtr)();

MemberPtr global_nonvirt_ptr = &A::nonvirt;
MemberPtr global_virt_ptr = &A::virt;
int (*global_fn_ptr)() = &global_fn;

// CHECK: @global_nonvirt_ptr = addrspace(200) global { i8 addrspace(200)*, i64 } { i8 addrspace(200)* addrspacecast (i8* bitcast (i32 (%class.A addrspace(200)*)* @_ZN1A7nonvirtEv to i8*) to i8 addrspace(200)*), i64 0 }, align [[$CAP_SIZE:16|32]]
// CHECK: @global_virt_ptr = addrspace(200) global { i8 addrspace(200)*, i64 } { i8 addrspace(200)* null, i64 1 }, align [[$CAP_SIZE]]
// CHECK: @global_fn_ptr = addrspace(200) global i32 () addrspace(200)* addrspacecast (i32 ()* @_Z9global_fnv to i32 () addrspace(200)*), align [[$CAP_SIZE]]

int call_nonvirt(A* a) {
  // CHECK: load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* @global_nonvirt_ptr, align [[$CAP_SIZE]]
  return (a->*global_nonvirt_ptr)();
}

int call_virt(A* a) {
  // CHECK: load { i8 addrspace(200)*, i64 }, { i8 addrspace(200)*, i64 } addrspace(200)* @global_virt_ptr, align [[$CAP_SIZE]]
  return (a->*global_virt_ptr)();
}

int call_local_nonvirt(A* a) {
  MemberPtr local_nonvirt = &A::nonvirt2;
  // FIXME: should we rather memcopy from a global that has been initialized?
  // This way we don't need to be able to derive it from PCC
  // CHECK: call i8 addrspace(200)* @llvm.cheri.pcc.get()
  // CHECK: call i8 addrspace(200)* @llvm.cheri.cap.offset.set(i8 addrspace(200)* %{{.+}}, i64 ptrtoint (i32 (%class.A addrspace(200)*)* @_ZN1A8nonvirt2Ev to i64))
  return (a->*local_nonvirt)();
}

int call_local_virt(A* a) {
  MemberPtr local_virt = &A::virt2;
  // CHECK: store { i8 addrspace(200)*, i64 } { i8 addrspace(200)* inttoptr (i64 32 to i8 addrspace(200)*), i64 1 }, { i8 addrspace(200)*, i64 } addrspace(200)* %{{.+}}, align [[$CAP_SIZE]]
  return (a->*local_virt)();
}

int call_local_fn_ptr(A* a) {
  int (*local_fn_ptr)() = &global_fn;
  // CHECK: call i8 addrspace(200)* @llvm.cheri.pcc.get()
  // CHECK: call i8 addrspace(200)* @llvm.cheri.cap.offset.set(i8 addrspace(200)* %{{.+}}, i64 ptrtoint (i32 ()* @_Z9global_fnv to i64))
  return local_fn_ptr();
}

int main() {
  A a;
  global_fn_ptr();
  call_virt(&a);
  call_nonvirt(&a);
  call_virt(&a);
}

// RELOCS:      Section (22) .rela.data {
// RELOCS-NEXT:   0x0 R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE _ZN1A7nonvirtEv 0x0
// RELOCS-NEXT:   0x{{4|8}}0 R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE _Z9global_fnv 0x0
// RELOCS-NEXT: }
// RELOCS-NEXT: Section (25) .rela.data.rel.ro._ZTV1A {
// RELOCS-NEXT:   0x{{1|2}}0 R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE _ZTI1A 0x0
// RELOCS-NEXT:   0x{{2|4}}0 R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE _ZN1A4virtEv 0x0
// RELOCS-NEXT:   0x{{3|6}}0 R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE _ZN1A5virt2Ev 0x0
// RELOCS-NEXT: }
// RELOCS-NEXT: Section (30) .rela.data.rel.ro._ZTI1A {
// RELOCS-NEXT:   0x0 R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE _ZTVN10__cxxabiv117__class_type_infoE 0x40
// RELOCS-NEXT:   0x{{1|2}}0 R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE _ZTS1A 0x0
// RELOCS-NEXT: }