int ecc_map(ecc_point* P, mp_int* modulus, mp_digit mp)
{
#ifndef WOLFSSL_SP_MATH
#ifdef WOLFSSL_SMALL_STACK
   mp_int* t1 = NULL;
   mp_int* t2 = NULL;
#ifdef ALT_ECC_SIZE
   mp_int* rx = NULL;
   mp_int* ry = NULL;
   mp_int* rz = NULL;
#endif
#else
   mp_int  t1[1], t2[1];
#ifdef ALT_ECC_SIZE
   mp_int  rx[1], ry[1], rz[1];
#endif
#endif /* WOLFSSL_SMALL_STACK */
   mp_int *x, *y, *z;
   int    err;

   if (P == NULL || modulus == NULL)
       return ECC_BAD_ARG_E;

   /* special case for point at infinity */
   if (mp_cmp_d(P->z, 0) == MP_EQ) {
       err = mp_set(P->x, 0);
       if (err == MP_OKAY)
           err = mp_set(P->y, 0);
       if (err == MP_OKAY)
           err = mp_set(P->z, 1);
       return err;
   }

#ifdef WOLFSSL_SMALL_STACK
#ifdef WOLFSSL_SMALL_STACK_CACHE
   if (P->key != NULL) {
       t1 = P->key->t1;
       t2 = P->key->t2;
   #ifdef ALT_ECC_SIZE
       rx = P->key->x;
       ry = P->key->y;
       rz = P->key->z;
   #endif
   }
   else
#endif /* WOLFSSL_SMALL_STACK_CACHE */
   {
       t1 = (mp_int*)XMALLOC(sizeof(mp_int), NULL, DYNAMIC_TYPE_ECC);
       t2 = (mp_int*)XMALLOC(sizeof(mp_int), NULL, DYNAMIC_TYPE_ECC);
       if (t1 == NULL || t2 == NULL) {
           XFREE(t2, NULL, DYNAMIC_TYPE_ECC);
           XFREE(t1, NULL, DYNAMIC_TYPE_ECC);
           return MEMORY_E;
       }
#ifdef ALT_ECC_SIZE
       rx = (mp_int*)XMALLOC(sizeof(mp_int), NULL, DYNAMIC_TYPE_ECC);
       ry = (mp_int*)XMALLOC(sizeof(mp_int), NULL, DYNAMIC_TYPE_ECC);
       rz = (mp_int*)XMALLOC(sizeof(mp_int), NULL, DYNAMIC_TYPE_ECC);
       if (rx == NULL || ry == NULL || rz == NULL) {
           XFREE(rz, NULL, DYNAMIC_TYPE_ECC);
           XFREE(ry, NULL, DYNAMIC_TYPE_ECC);
           XFREE(rx, NULL, DYNAMIC_TYPE_ECC);
           XFREE(t2, NULL, DYNAMIC_TYPE_ECC);
           XFREE(t1, NULL, DYNAMIC_TYPE_ECC);
           return MEMORY_E;
       }
#endif
   }
#endif /* WOLFSSL_SMALL_STACK */

   if ((err = mp_init_multi(t1, t2, NULL, NULL, NULL, NULL)) != MP_OKAY) {
#ifdef WOLFSSL_SMALL_STACK
#ifdef WOLFSSL_SMALL_STACK_CACHE
      if (P->key == NULL)
#endif
      {
      #ifdef ALT_ECC_SIZE
         XFREE(rz, NULL, DYNAMIC_TYPE_ECC);
         XFREE(ry, NULL, DYNAMIC_TYPE_ECC);
         XFREE(rx, NULL, DYNAMIC_TYPE_ECC);
      #endif
         XFREE(t2, NULL, DYNAMIC_TYPE_ECC);
         XFREE(t1, NULL, DYNAMIC_TYPE_ECC);
      }
#endif
      return MEMORY_E;
   }

#ifdef ALT_ECC_SIZE
   /* Use local stack variable */
   x = rx;
   y = ry;
   z = rz;

   if ((err = mp_init_multi(x, y, z, NULL, NULL, NULL)) != MP_OKAY) {
       goto done;
   }

   if (err == MP_OKAY)
       err = mp_copy(P->x, x);
   if (err == MP_OKAY)
       err = mp_copy(P->y, y);
   if (err == MP_OKAY)
       err = mp_copy(P->z, z);

   if (err != MP_OKAY) {
      goto done;
   }
#else
   /* Use destination directly */
   x = P->x;
   y = P->y;
   z = P->z;
#endif

   /* first map z back to normal */
   err = mp_montgomery_reduce(z, modulus, mp);

   /* get 1/z */
   if (err == MP_OKAY)
       err = mp_invmod(z, modulus, t1);

   /* get 1/z^2 and 1/z^3 */
   if (err == MP_OKAY)
       err = mp_sqr(t1, t2);
   if (err == MP_OKAY)
       err = mp_mod(t2, modulus, t2);
   if (err == MP_OKAY)
       err = mp_mul(t1, t2, t1);
   if (err == MP_OKAY)
       err = mp_mod(t1, modulus, t1);

   /* multiply against x/y */
   if (err == MP_OKAY)
       err = mp_mul(x, t2, x);
   if (err == MP_OKAY)
       err = mp_montgomery_reduce(x, modulus, mp);
   if (err == MP_OKAY)
       err = mp_mul(y, t1, y);
   if (err == MP_OKAY)
       err = mp_montgomery_reduce(y, modulus, mp);

   if (err == MP_OKAY)
       err = mp_set(z, 1);

#ifdef ALT_ECC_SIZE
   /* return result */
   if (err == MP_OKAY)
      err = mp_copy(x, P->x);
   if (err == MP_OKAY)
      err = mp_copy(y, P->y);
   if (err == MP_OKAY)
      err = mp_copy(z, P->z);

done:
#endif

   /* clean up */
   mp_clear(t1);
   mp_clear(t2);

#ifdef WOLFSSL_SMALL_STACK
#ifdef WOLFSSL_SMALL_STACK_CACHE
   if (P->key == NULL)
#endif
   {
   #ifdef ALT_ECC_SIZE
      XFREE(rz, NULL, DYNAMIC_TYPE_ECC);
      XFREE(ry, NULL, DYNAMIC_TYPE_ECC);
      XFREE(rx, NULL, DYNAMIC_TYPE_ECC);
   #endif
      XFREE(t2, NULL, DYNAMIC_TYPE_ECC);
      XFREE(t1, NULL, DYNAMIC_TYPE_ECC);
   }
#endif

   return err;
#else
    if (P == NULL || modulus == NULL)
        return ECC_BAD_ARG_E;

    (void)mp;

#ifndef WOLFSSL_SP_NO_256
    if (mp_count_bits(modulus) == 256) {
        return sp_ecc_map_256(P->x, P->y, P->z);
    }
#endif
#ifdef WOLFSSL_SP_384
    if (mp_count_bits(modulus) == 384) {
        return sp_ecc_map_384(P->x, P->y, P->z);
    }
#endif
    return ECC_BAD_ARG_E;
#endif
}